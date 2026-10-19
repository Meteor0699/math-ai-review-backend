#include "services/AgentService.h"

#include <drogon/drogon.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/MysqlSyncClient.h"
#include "utils/TextUtil.h"

namespace mathai::services
{
namespace
{

// The history window and the per-payload budget live in AgentDao / TextUtil so
// the model context and the SSE frames cannot drift apart.
constexpr std::size_t maximumUserMessageLength = 2000;
constexpr int toolDefaultLimit = 10;

std::string quote(const std::string &value) { return mathai::utils::mysql::quote(value); }

std::string trim(const std::string &value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
                         return std::isspace(ch) != 0;
                     }).base();
    if (begin >= end)
    {
        return "";
    }
    return std::string(begin, end);
}

std::optional<long long> optionalId(const Json::Value &args, const char *field)
{
    if (!args.isMember(field) || args[field].isNull())
    {
        return std::nullopt;
    }
    const auto &value = args[field];
    if (value.isInt64() || value.isUInt64() || value.isInt() || value.isUInt())
    {
        const auto number = value.asInt64();
        return number > 0 ? std::optional<long long>(number) : std::nullopt;
    }
    if (value.isString())
    {
        try
        {
            const auto number = std::stoll(value.asString());
            return number > 0 ? std::optional<long long>(number) : std::nullopt;
        }
        catch (const std::exception &)
        {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::string stringArg(const Json::Value &args, const char *field)
{
    if (!args.isMember(field) || !args[field].isString())
    {
        return "";
    }
    return args[field].asString();
}

bool allowedValue(const std::string &value, const std::unordered_set<std::string> &allowed)
{
    return allowed.find(value) != allowed.end();
}

// Model-supplied numeric knobs are clamped, never rejected: a hallucinated
// `limit` must not fail the turn.
int limitArg(const Json::Value &args, const char *field, int fallback, int cap)
{
    if (!args.isMember(field) || args[field].isNull())
    {
        return fallback;
    }
    int value = fallback;
    if (args[field].isIntegral())
    {
        value = args[field].asInt();
    }
    else if (args[field].isString())
    {
        try
        {
            value = std::stoi(args[field].asString());
        }
        catch (const std::exception &)
        {
            return fallback;
        }
    }
    else
    {
        return fallback;
    }
    return std::clamp(value, 1, cap);
}

std::string likeLiteral(const std::string &keyword)
{
    // % and _ are LIKE wildcards; keep them out of the user-supplied term so a
    // one-character keyword cannot turn into a full table scan on `%`.
    std::string cleaned;
    cleaned.reserve(keyword.size());
    for (const auto ch : keyword)
    {
        if (ch == '%' || ch == '_' || ch == '\\')
        {
            continue;
        }
        cleaned.push_back(ch);
    }
    return cleaned;
}

// The reserved assistant row starts empty and is filled when the turn settles.
// The stored value is the plain text whenever the turn stayed conversational;
// once tool calls or hidden reasoning are involved it becomes a JSON envelope so
// the history can be replayed as a valid assistant + tool message pair.
struct AgentAnswer
{
    std::string content;
    std::string reasoning;
    Json::Value toolCalls{Json::arrayValue};
};

std::string serializeAnswer(const AgentAnswer &answer)
{
    const auto hasToolCalls = answer.toolCalls.isArray() && !answer.toolCalls.empty();
    if (!hasToolCalls && answer.reasoning.empty())
    {
        return answer.content;
    }

    Json::Value stored(Json::objectValue);
    stored["text"] = answer.content;
    if (hasToolCalls)
    {
        stored["toolCalls"] = answer.toolCalls;
    }
    if (!answer.reasoning.empty())
    {
        stored["reasoningContent"] = answer.reasoning;
    }
    return mathai::utils::jsonToCompactString(stored);
}

void persistPartial(const mathai::dao::AgentDao &dao,
                    long long assistantMessageId,
                    const AgentAnswer &answer,
                    bool bestEffort,
                    std::string *failure = nullptr)
{
    dao.updateMessageContent(assistantMessageId, serializeAnswer(answer),
                             [failure, bestEffort](const std::string &message) {
        LOG_ERROR << "AgentService: failed to persist the assistant answer: " << message;
        if (failure != nullptr)
        {
            *failure = message;
        }
        else if (!bestEffort)
        {
            LOG_ERROR << "AgentService: the answer could not be persisted";
        }
    });
}

Json::Value cappedResult(const Json::Value &value)
{
    return mathai::utils::parseJsonLoose(
        mathai::utils::truncateUtf8(mathai::utils::jsonToCompactString(value),
                                    mathai::utils::agentPayloadCapBytes));
}

Json::Value questionItem(const mathai::utils::mysql::Row &row)
{
    Json::Value item;
    item["id"] = Json::Int64(row.getInt64("id"));
    item["title"] = row.getString("title");
    item["questionType"] = row.getString("question_type");
    item["difficulty"] = row.getString("difficulty");
    item["courseName"] = row.getString("course_name");
    return item;
}

} // namespace

// ---------------------------------------------------------------------------
// Tools: read-only, synchronous, executed on the AI worker thread via the
// shared MySQL pool. The current user id is injected by the server and is never
// read from the model arguments.
// ---------------------------------------------------------------------------
namespace tools
{
namespace
{

Json::Value searchQuestions(long long /*userId*/, const Json::Value &args)
{
    static const std::unordered_set<std::string> allowedTypes = {
        "single_choice", "blank", "calculation", "solution", "proof"};
    static const std::unordered_set<std::string> allowedDifficulties = {"basic", "medium", "hard"};

    std::ostringstream where;
    where << " WHERE q.status = 1";
    const auto keyword = trim(likeLiteral(stringArg(args, "keyword")));
    if (!keyword.empty())
    {
        if (keyword.size() > 100)
        {
            throw std::runtime_error("keyword is too long");
        }
        const auto pattern = quote("%" + keyword + "%");
        where << " AND (q.title LIKE " << pattern << " OR q.content LIKE " << pattern << ")";
    }
    if (const auto courseId = optionalId(args, "courseId"))
    {
        where << " AND q.course_id = " << *courseId;
    }
    if (const auto chapterId = optionalId(args, "chapterId"))
    {
        where << " AND q.chapter_id = " << *chapterId;
    }
    const auto difficulty = stringArg(args, "difficulty");
    if (!difficulty.empty())
    {
        if (!allowedValue(difficulty, allowedDifficulties))
        {
            throw std::runtime_error("difficulty must be one of basic, medium, hard");
        }
        where << " AND q.difficulty = " << quote(difficulty);
    }
    const auto questionType = stringArg(args, "questionType");
    if (!questionType.empty())
    {
        if (!allowedValue(questionType, allowedTypes))
        {
            throw std::runtime_error("questionType is not a supported question type");
        }
        where << " AND q.question_type = " << quote(questionType);
    }

    const auto limit = limitArg(args, "limit", toolDefaultLimit, 20);
    const auto from = std::string(" FROM question q LEFT JOIN course c ON q.course_id = c.id ") + where.str();

    const auto countResult = mathai::utils::mysql::execute("SELECT COUNT(*) AS total" + from);
    const auto total = countResult.rows.empty() ? 0 : countResult.rows[0].getInt64("total");

    const auto result = mathai::utils::mysql::execute(
        "SELECT q.id, q.title, q.question_type, q.difficulty, c.name AS course_name" + from +
        " ORDER BY q.id LIMIT " + std::to_string(limit));

    Json::Value items(Json::arrayValue);
    for (const auto &row : result.rows)
    {
        items.append(questionItem(row));
    }

    Json::Value data;
    data["items"] = items;
    data["total"] = Json::UInt64(total);
    return data;
}

Json::Value getQuestionAnswer(long long /*userId*/, const Json::Value &args)
{
    const auto questionId = optionalId(args, "questionId");
    if (!questionId)
    {
        throw std::runtime_error("questionId is required");
    }

    const auto result = mathai::utils::mysql::execute(
        "SELECT standard_answer, normal_explanation FROM question WHERE id = " + std::to_string(*questionId) +
        " AND status = 1 LIMIT 1");
    if (result.rows.empty())
    {
        throw std::runtime_error("question not found");
    }

    Json::Value data;
    data["standardAnswer"] = result.rows[0].getString("standard_answer");
    data["normalExplanation"] = result.rows[0].getString("normal_explanation");
    return data;
}

Json::Value listKnowledgePoints(long long /*userId*/, const Json::Value &args)
{
    std::ostringstream where;
    where << " WHERE kp.status = 1";
    if (const auto courseId = optionalId(args, "courseId"))
    {
        where << " AND kp.course_id = " << *courseId;
    }
    if (const auto chapterId = optionalId(args, "chapterId"))
    {
        where << " AND kp.chapter_id = " << *chapterId;
    }

    const auto limit = limitArg(args, "limit", toolDefaultLimit, 20);
    const auto result = mathai::utils::mysql::execute(
        "SELECT kp.id, kp.title, kp.exam_frequency, ch.title AS chapter_name FROM knowledge_point kp "
        "LEFT JOIN chapter ch ON kp.chapter_id = ch.id" + where.str() +
        " ORDER BY kp.sort_order, kp.id LIMIT " + std::to_string(limit));

    Json::Value items(Json::arrayValue);
    for (const auto &row : result.rows)
    {
        Json::Value item;
        item["id"] = Json::Int64(row.getInt64("id"));
        item["title"] = row.getString("title");
        item["chapterName"] = row.getString("chapter_name");
        item["examFrequency"] = row.getString("exam_frequency");
        items.append(item);
    }

    Json::Value data;
    data["items"] = items;
    return data;
}

Json::Value listMyWrongQuestions(long long userId, const Json::Value &args)
{
    const auto limit = limitArg(args, "limit", toolDefaultLimit, 20);
    const auto result = mathai::utils::mysql::execute(
        "SELECT wq.question_id, wq.added_at, q.title, c.name AS course_name FROM wrong_question wq "
        "JOIN question q ON wq.question_id = q.id LEFT JOIN course c ON q.course_id = c.id "
        "WHERE wq.user_id = " + std::to_string(userId) + " AND wq.status = 1 AND q.status = 1 "
        "ORDER BY wq.added_at DESC, wq.id DESC LIMIT " + std::to_string(limit));

    Json::Value items(Json::arrayValue);
    for (const auto &row : result.rows)
    {
        Json::Value item;
        item["questionId"] = Json::Int64(row.getInt64("question_id"));
        item["title"] = row.getString("title");
        item["courseName"] = row.getString("course_name");
        item["addedAt"] = row.getString("added_at");
        items.append(item);
    }

    Json::Value data;
    data["items"] = items;
    return data;
}

Json::Value listMyStudyRecords(long long userId, const Json::Value &args)
{
    const auto limit = limitArg(args, "limit", toolDefaultLimit, 20);
    const auto result = mathai::utils::mysql::execute(
        "SELECT qa.question_id, qa.status, qa.last_attempt_at, qa.attempt_count, q.title FROM question_attempt qa "
        "JOIN question q ON qa.question_id = q.id "
        "WHERE qa.user_id = " + std::to_string(userId) + " AND q.status = 1 "
        "ORDER BY qa.last_attempt_at DESC, qa.id DESC LIMIT " + std::to_string(limit));

    Json::Value items(Json::arrayValue);
    for (const auto &row : result.rows)
    {
        Json::Value item;
        item["questionId"] = Json::Int64(row.getInt64("question_id"));
        item["title"] = row.getString("title");
        item["status"] = row.getString("status");
        item["lastAttemptAt"] = row.getString("last_attempt_at");
        item["attemptCount"] = row.getInt("attempt_count");
        items.append(item);
    }

    Json::Value data;
    data["items"] = items;
    return data;
}

Json::Value buildPracticeSet(long long /*userId*/, const Json::Value &args)
{
    static const std::unordered_set<std::string> allowedDifficulties = {"basic", "medium", "hard"};

    std::ostringstream where;
    where << " WHERE q.status = 1";
    if (const auto courseId = optionalId(args, "courseId"))
    {
        where << " AND q.course_id = " << *courseId;
    }
    if (const auto chapterId = optionalId(args, "chapterId"))
    {
        where << " AND q.chapter_id = " << *chapterId;
    }
    const auto difficulty = stringArg(args, "difficulty");
    if (!difficulty.empty())
    {
        if (!allowedValue(difficulty, allowedDifficulties))
        {
            throw std::runtime_error("difficulty must be one of basic, medium, hard");
        }
        where << " AND q.difficulty = " << quote(difficulty);
    }

    const auto count = limitArg(args, "count", 5, 10);
    // Draw from a slightly wider pool than requested, then shuffle in memory:
    // the statement stays simple and no answer column is ever selected.
    const auto pool = std::max(count * 3, count);
    auto result = mathai::utils::mysql::execute(
        "SELECT q.id, q.title, q.difficulty, c.name AS course_name FROM question q "
        "LEFT JOIN course c ON q.course_id = c.id" + where.str() +
        " ORDER BY q.id LIMIT " + std::to_string(pool));

    std::vector<mathai::utils::mysql::Row> rows(result.rows.begin(), result.rows.end());
    std::shuffle(rows.begin(), rows.end(), std::mt19937(std::random_device{}()));

    Json::Value items(Json::arrayValue);
    for (const auto &row : rows)
    {
        if (static_cast<int>(items.size()) >= count)
        {
            break;
        }
        Json::Value item;
        item["id"] = Json::Int64(row.getInt64("id"));
        item["title"] = row.getString("title");
        item["difficulty"] = row.getString("difficulty");
        item["courseName"] = row.getString("course_name");
        items.append(item);
    }

    Json::Value data;
    data["items"] = items;
    return data;
}

} // namespace

Json::Value executeTool(const std::string &name, long long userId, const Json::Value &args)
{
    if (name == "search_questions") return searchQuestions(userId, args);
    if (name == "get_question_answer") return getQuestionAnswer(userId, args);
    if (name == "list_knowledge_points") return listKnowledgePoints(userId, args);
    if (name == "list_my_wrong_questions") return listMyWrongQuestions(userId, args);
    if (name == "list_my_study_records") return listMyStudyRecords(userId, args);
    if (name == "build_practice_set") return buildPracticeSet(userId, args);
    throw std::runtime_error("unknown tool: " + name);
}

} // namespace tools

namespace
{

std::vector<mathai::dao::AgentMessage> loadContextHistory(const mathai::dao::AgentDao &dao,
                                                          long long conversationId,
                                                          long long excludeMessageId)
{
    std::vector<mathai::dao::AgentMessage> history;
    std::mutex mutex;
    std::condition_variable finished;
    bool ready = false;

    dao.loadRecentMessages(
        conversationId,
        40,
        excludeMessageId,
        [&history, &mutex, &finished, &ready](std::vector<mathai::dao::AgentMessage> messages) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                history = std::move(messages);
                ready = true;
            }
            finished.notify_all();
        },
        [&mutex, &finished, &ready](const std::string &message) {
            LOG_ERROR << "AgentService: failed to load history: " << message;
            {
                std::lock_guard<std::mutex> lock(mutex);
                ready = true;
            }
            finished.notify_all();
        });

    std::unique_lock<std::mutex> lock(mutex);
    finished.wait_for(lock, std::chrono::seconds(10), [&ready] { return ready; });
    return history;
}

std::string questionContext(long long questionId)
{
    const auto result = mathai::utils::mysql::execute(
        "SELECT q.title, q.content, q.question_type, q.standard_answer, q.normal_explanation, c.name AS course_name, "
        "ch.title AS chapter_name FROM question q LEFT JOIN course c ON q.course_id = c.id "
        "LEFT JOIN chapter ch ON q.chapter_id = ch.id WHERE q.id = " + std::to_string(questionId) +
        " AND q.status = 1 LIMIT 1");
    if (result.rows.empty())
    {
        return "";
    }

    const auto &row = result.rows.front();
    std::ostringstream context;
    context << "课程：" << row.getString("course_name") << "\n"
            << "章节：" << row.getString("chapter_name") << "\n"
            << "题型：" << row.getString("question_type") << "\n"
            << "题目：" << mathai::utils::truncateUtf8(row.getString("content"), 2000) << "\n"
            << "标准答案：" << mathai::utils::truncateUtf8(row.getString("standard_answer"), 1000) << "\n"
            << "普通解析：" << mathai::utils::truncateUtf8(row.getString("normal_explanation"), 1500);
    return context.str();
}

std::string systemPrompt(const std::string &context)
{
    std::ostringstream prompt;
    prompt << "你是高校数学复习平台里题目页的 AI 学习助手，正在和一名学生围绕一道题进行多轮对话。\n"
           << "要求：\n"
           << "1. 回答要围绕当前题目和高等数学、线性代数、概率论等大学数学课程，简洁清楚，适合期末复习。\n"
           << "2. 不要编造题目里不存在的条件；信息不足时说明依据现有条件能推到哪一步。\n"
           << "3. 数学公式使用 LaTeX，行内公式用 \\(...\\)，块级公式用 \\[...\\]。\n"
           << "4. 需要查资料时调用提供的工具，不要凭空猜测题库内容；工具返回的是系统数据，直接采用。\n"
           << "5. 如果学生问的内容与数学学习无关，只回复一句：这个问题和当前数学学习无关，我只能回答与本题或数学复习相关的问题。\n\n"
           << "当前题目信息：\n"
           << (context.empty() ? std::string("（题目信息不可用）") : context);
    return prompt.str();
}

// Converts stored messages back into the OpenAI chat format. The reserved
// assistant row (empty content) and the tool rows created for one model call
// are glued back into a single assistant message with tool_calls, because that
// ordering is what the provider requires.
void appendModelHistory(const std::vector<mathai::dao::AgentMessage> &history, Json::Value &messages)
{
    std::size_t index = 0;
    while (index < history.size())
    {
        const auto &message = history[index];

        if (message.role == "user")
        {
            Json::Value entry(Json::objectValue);
            entry["role"] = "user";
            entry["content"] = message.content;
            messages.append(entry);
            ++index;
            continue;
        }

        if (message.role == "assistant")
        {
            // Content may be a plain string (no tools) or a JSON envelope that
            // also carries the tool calls of the turn and the hidden reasoning
            // the provider expects back alongside them.
            auto assistantText = message.content;
            std::string assistantReasoning;
            Json::Value storedCalls(Json::arrayValue);
            if (!message.content.empty() && message.content.front() == '{')
            {
                const auto parsed = mathai::utils::parseJsonLoose(message.content);
                if (parsed.isObject() && parsed.isMember("text"))
                {
                    assistantText = parsed["text"].isString() ? parsed["text"].asString() : "";
                    if (parsed.isMember("reasoningContent") && parsed["reasoningContent"].isString())
                    {
                        assistantReasoning = parsed["reasoningContent"].asString();
                    }
                    if (parsed.isMember("toolCalls") && parsed["toolCalls"].isArray())
                    {
                        storedCalls = parsed["toolCalls"];
                    }
                }
            }

            Json::Value toolCalls(Json::arrayValue);
            auto cursor = index + 1;
            while (cursor < history.size() && history[cursor].role == "tool")
            {
                const auto args = mathai::utils::parseJsonLoose(history[cursor].toolArgs);
                const auto callId = args.isObject() && args.isMember("toolCallId") && args["toolCallId"].isString()
                                        ? args["toolCallId"].asString()
                                        : "call_" + std::to_string(history[cursor].id);
                Json::Value call(Json::objectValue);
                call["id"] = callId;
                call["type"] = "function";
                call["function"]["name"] = history[cursor].toolName;
                call["function"]["arguments"] =
                    args.isObject() && args.isMember("arguments")
                        ? mathai::utils::jsonToCompactString(args["arguments"])
                        : std::string("{}");
                toolCalls.append(call);
                ++cursor;
            }

            // Order matters: the envelope holds the calls in the order the model
            // made them, and the tool rows follow in the same order. Prefer the
            // envelope and fall back to the rows when it is missing.
            if (storedCalls.size() == toolCalls.size())
            {
                toolCalls = storedCalls;
            }
            // Drop calls whose tool row is missing (an interrupted round) so the
            // provider never sees half a pair.
            if (toolCalls.size() > static_cast<Json::ArrayIndex>(cursor - index - 1))
            {
                Json::Value trimmed(Json::arrayValue);
                const auto available = static_cast<Json::ArrayIndex>(cursor - index - 1);
                for (Json::ArrayIndex i = 0; i < available && i < toolCalls.size(); ++i)
                {
                    trimmed.append(toolCalls[i]);
                }
                toolCalls = trimmed;
            }

            Json::Value entry(Json::objectValue);
            entry["role"] = "assistant";
            entry["content"] = assistantText;
            // DeepSeek's thinking mode requires the reasoning to be echoed back
            // on every replayed assistant message, tool calls or not. Turns that
            // predate this handling have none stored, so the field is sent empty
            // rather than omitted. It is never surfaced to the student.
            entry["reasoning_content"] = assistantReasoning;
            if (!toolCalls.empty())
            {
                entry["tool_calls"] = toolCalls;
            }
            messages.append(entry);

            for (std::size_t callIndex = index + 1; callIndex < cursor; ++callIndex)
            {
                Json::Value toolEntry(Json::objectValue);
                toolEntry["role"] = "tool";
                toolEntry["tool_call_id"] = toolCalls[static_cast<Json::ArrayIndex>(callIndex - index - 1)]["id"];
                toolEntry["content"] = mathai::utils::truncateUtf8(history[callIndex].toolResult,
                                                                 mathai::utils::agentPayloadCapBytes);
                messages.append(toolEntry);
            }
            index = cursor;
            continue;
        }

        // Orphaned tool row (for example after a partial failure): keep the
        // information but do not emit an invalid tool message without its call.
        Json::Value entry(Json::objectValue);
        entry["role"] = "user";
        entry["content"] = "工具 " + message.toolName + " 返回：" +
                           mathai::utils::truncateUtf8(message.toolResult, 1024);
        messages.append(entry);
        ++index;
    }
}

} // namespace

Json::Value AgentService::toolSchema()
{
    Json::Value tools(Json::arrayValue);

    const auto add = [&tools](const std::string &name, const std::string &description, const Json::Value &properties,
                              const Json::Value &required) {
        Json::Value function(Json::objectValue);
        function["name"] = name;
        function["description"] = description;

        Json::Value parameters(Json::objectValue);
        parameters["type"] = "object";
        parameters["properties"] = properties;
        parameters["required"] = required;
        function["parameters"] = parameters;

        Json::Value entry(Json::objectValue);
        entry["type"] = "function";
        entry["function"] = function;
        tools.append(entry);
    };

    const auto integerProperty = [](const std::string &description) {
        Json::Value property(Json::objectValue);
        property["type"] = "integer";
        property["description"] = description;
        return property;
    };
    const auto stringProperty = [](const std::string &description) {
        Json::Value property(Json::objectValue);
        property["type"] = "string";
        property["description"] = description;
        return property;
    };
    const auto enumerationProperty = [](const std::string &description, const std::vector<std::string> &values) {
        Json::Value property(Json::objectValue);
        property["type"] = "string";
        property["description"] = description;
        Json::Value allowed(Json::arrayValue);
        for (const auto &value : values)
        {
            allowed.append(value);
        }
        property["enum"] = allowed;
        return property;
    };

    {
        Json::Value properties(Json::objectValue);
        properties["keyword"] = stringProperty("题目关键词，可为空");
        properties["courseId"] = integerProperty("课程ID，可为空");
        properties["chapterId"] = integerProperty("章节ID，可为空");
        properties["difficulty"] = enumerationProperty("难度，可为空", {"basic", "medium", "hard"});
        properties["questionType"] =
            enumerationProperty("题型，可为空", {"single_choice", "blank", "calculation", "solution", "proof"});
        properties["limit"] = integerProperty("返回条数，最大20，默认10");
        add("search_questions", "按关键词/课程/章节/难度/题型检索题库，只返回题目摘要，不含答案。", properties,
            Json::Value(Json::arrayValue));
    }
    {
        Json::Value properties(Json::objectValue);
        properties["questionId"] = integerProperty("题目ID，必填");
        Json::Value required(Json::arrayValue);
        required.append("questionId");
        add("get_question_answer", "读取某道题的标准答案与普通解析。", properties, required);
    }
    {
        Json::Value properties(Json::objectValue);
        properties["courseId"] = integerProperty("课程ID，可为空");
        properties["chapterId"] = integerProperty("章节ID，可为空");
        properties["limit"] = integerProperty("返回条数，最大20，默认10");
        add("list_knowledge_points", "列出重点知识点及其章节和考频。", properties, Json::Value(Json::arrayValue));
    }
    {
        Json::Value properties(Json::objectValue);
        properties["limit"] = integerProperty("返回条数，最大20，默认10");
        add("list_my_wrong_questions", "列出当前学生错题本里的题目。", properties, Json::Value(Json::arrayValue));
    }
    {
        Json::Value properties(Json::objectValue);
        properties["limit"] = integerProperty("返回条数，最大20，默认10");
        add("list_my_study_records", "列出当前学生的学习记录与掌握状态。", properties, Json::Value(Json::arrayValue));
    }
    {
        Json::Value properties(Json::objectValue);
        properties["courseId"] = integerProperty("课程ID，可为空");
        properties["chapterId"] = integerProperty("章节ID，可为空");
        properties["difficulty"] = enumerationProperty("难度，可为空", {"basic", "medium", "hard"});
        properties["count"] = integerProperty("抽题数量，最大10，默认5");
        add("build_practice_set", "按条件抽取一组练习题，只返回题目摘要，不含答案。", properties,
            Json::Value(Json::arrayValue));
    }

    return tools;
}

bool AgentService::hasCapacity()
{
    return mathai::clients::AiClient::hasCapacity();
}

void AgentService::loadSession(long long userId,
                               long long questionId,
                               std::function<void(long long, Json::Value)> onSuccess,
                               std::function<void(const std::string &)> onError) const
{
    const auto conversationId = [&]() -> std::optional<long long> {
        std::optional<long long> found;
        agentDao_.findConversationIdForUser(
            userId,
            questionId,
            [&found](long long id) { found = id; },
            [&found](const std::string &message) {
                LOG_ERROR << "AgentService: failed to resolve conversation: " << message;
                found = std::nullopt;
            });
        return found;
    }();

    if (!conversationId)
    {
        onError("database error");
        return;
    }

    Json::Value messages(Json::arrayValue);
    if (*conversationId == 0)
    {
        onSuccess(0, messages);
        return;
    }

    std::vector<mathai::dao::AgentMessage> history;
    std::string failure;
    agentDao_.loadHistory(
        *conversationId,
        [&history](std::vector<mathai::dao::AgentMessage> loaded) { history = std::move(loaded); },
        [&failure](const std::string &message) { failure = message; });
    if (!failure.empty())
    {
        onError(failure);
        return;
    }

    for (const auto &message : history)
    {
        Json::Value item(Json::objectValue);
        item["id"] = Json::Int64(message.id);
        item["role"] = message.role;
        item["content"] = message.content;
        if (message.role == "tool")
        {
            item["toolName"] = message.toolName;
            const auto args = mathai::utils::parseJsonLoose(message.toolArgs);
            item["toolArgs"] = args.isObject() && args.isMember("arguments") ? args["arguments"] : args;
            item["toolResult"] = mathai::utils::parseJsonLoose(message.toolResult);
        }
        item["createdAt"] = message.createdAt;
        messages.append(item);
    }
    onSuccess(*conversationId, messages);
}

AgentTurnHandle AgentService::prepareTurn(const AgentTurnRequest &request,
                                          const AgentTurnCallbacks &callbacks) const
{
    AgentTurnHandle handle;
    const auto fail = [&handle, &callbacks](const std::string &message, const std::string &code) {
        handle.ok = false;
        handle.errorMessage = message;
        handle.errorCode = code;
        if (callbacks.onError)
        {
            callbacks.onError(message, code);
        }
        return handle;
    };

    const auto message = trim(request.message);
    if (message.empty())
    {
        return fail("message is required", "invalid_request");
    }
    if (message.size() > maximumUserMessageLength)
    {
        return fail("message is too long", "invalid_request");
    }

    // A full AI queue is reported before anything is written, so a rejected turn
    // never leaves a user message without an answer.
    if (!hasCapacity())
    {
        return fail("AI request queue is full", "busy");
    }

    bool questionFound = false;
    agentDao_.questionExists(
        request.questionId,
        [&questionFound](bool found) { questionFound = found; },
        [](const std::string &dbMessage) { LOG_ERROR << "AgentService: question lookup failed: " << dbMessage; });
    if (!questionFound)
    {
        return fail("question not found", "not_found");
    }

    long long conversationId = 0;
    std::string dbFailure;
    if (request.conversationId > 0)
    {
        // The client may echo a conversation id; it must belong to this user.
        agentDao_.findConversationIdForUser(
            request.userId,
            request.questionId,
            [&conversationId](long long id) { conversationId = id; },
            [&dbFailure](const std::string &value) { dbFailure = value; });
        if (!dbFailure.empty())
        {
            return fail("database error", "database_error");
        }
        if (conversationId != request.conversationId)
        {
            // A stale or foreign id is not fatal: fall back to the canonical
            // conversation for this (user, question) pair.
            conversationId = 0;
        }
    }

    if (conversationId == 0)
    {
        agentDao_.findOrCreateConversation(
            request.userId,
            request.questionId,
            [&conversationId](long long id) { conversationId = id; },
            [&dbFailure](const std::string &value) { dbFailure = value; });
        if (!dbFailure.empty() || conversationId == 0)
        {
            return fail("database error", "database_error");
        }
    }

    long long userMessageId = 0;
    agentDao_.appendMessage(
        conversationId,
        "user",
        message,
        "",
        "",
        "",
        [&userMessageId](long long id) { userMessageId = id; },
        [&dbFailure](const std::string &value) { dbFailure = value; });
    if (!dbFailure.empty() || userMessageId == 0)
    {
        return fail("database error", "database_error");
    }

    // Reserved assistant row: its id is known before generation, so the `meta`
    // frame can carry it. `created_at` is rewritten when the answer settles,
    // keeping the history ordered user -> assistant -> tools.
    long long assistantMessageId = 0;
    agentDao_.appendMessage(
        conversationId,
        "assistant",
        "",
        "",
        "",
        "",
        [&assistantMessageId](long long id) { assistantMessageId = id; },
        [&dbFailure](const std::string &value) { dbFailure = value; });
    if (!dbFailure.empty() || assistantMessageId == 0)
    {
        return fail("database error", "database_error");
    }

    handle.ok = true;
    handle.conversationId = conversationId;
    handle.userMessageId = userMessageId;
    handle.assistantMessageId = assistantMessageId;
    return handle;
}

// A turn is a chain of model calls. Nothing blocks: each model call is submitted
// to the shared AiExecutor and the callback for that call drives the next round,
// so the controller's IO thread only ever pays for the submit.
struct AgentTurnRunner
{
    const mathai::services::AgentService *service{nullptr};
    const mathai::dao::AgentDao *dao{nullptr};
    mathai::services::AgentTurnRequest request;
    mathai::services::AgentTurnCallbacks callbacks;
    mathai::services::AgentCancelFlag cancel;
    long long conversationId{0};
    long long assistantMessageId{0};
    Json::Value messages{Json::arrayValue};
    AgentAnswer answer;
    int promptTokens{0};
    int completionTokens{0};
    bool producedAnything{false};

    bool cancelled() const { return cancel && cancel->load(); }
};

// A round that never reports back (a hung socket, a lost callback) must not hold
// the stream open forever. The counter is shared with the round callback so the
// timeout cannot race the settle.
struct RoundGuard
{
    std::mutex mutex;
    std::condition_variable settledCondition;
    // Filled by the AiExecutor worker, consumed by the orchestrator thread.
    mathai::clients::AiStreamResult result;
    std::atomic_bool settled{false};
    std::atomic_bool reported{false};
};

void failTurn(const mathai::dao::AgentDao &dao,
              long long assistantMessageId,
              const AgentAnswer &answer,
              const mathai::services::AgentTurnCallbacks &callbacks,
              const std::string &message,
              const std::string &code)
{
    // Persist whatever was streamed before the failure; an empty answer still
    // closes the reserved placeholder so history never shows an empty bubble.
    AgentAnswer stored = answer;
    if (stored.content.empty())
    {
        stored.content = u8"\uff08\u672c\u8f6e\u6ca1\u6709\u751f\u6210\u5185\u5bb9\uff09";
    }
    persistPartial(dao, assistantMessageId, stored, false);

    if (callbacks.onError)
    {
        callbacks.onError(message, code);
    }
}

std::shared_ptr<AgentTurnRunner> makeTurnRunner(const mathai::services::AgentService *service,
                                                const mathai::dao::AgentDao *dao,
                                                const mathai::services::AgentTurnRequest &request,
                                                const mathai::services::AgentTurnCallbacks &callbacks,
                                                mathai::services::AgentCancelFlag cancel,
                                                long long conversationId,
                                                long long assistantMessageId,
                                                long long excludedUserMessageId)
{
    auto runner = std::make_shared<AgentTurnRunner>();
    runner->service = service;
    runner->dao = dao;
    runner->request = request;
    runner->callbacks = callbacks;
    runner->cancel = std::move(cancel);
    runner->conversationId = conversationId;
    runner->assistantMessageId = assistantMessageId;

    Json::Value system(Json::objectValue);
    system["role"] = "system";
    system["content"] = systemPrompt(questionContext(request.questionId));
    runner->messages.append(system);

    appendModelHistory(loadContextHistory(*dao, conversationId, excludedUserMessageId), runner->messages);
    return runner;
}

void finishTurn(const std::shared_ptr<AgentTurnRunner> &runner)
{
    std::string dbFailure;
    persistPartial(*runner->dao, runner->assistantMessageId, runner->answer,
                   false, &dbFailure);
    if (!dbFailure.empty())
    {
        if (runner->callbacks.onError)
        {
            runner->callbacks.onError("failed to persist the answer", "database_error");
        }
        return;
    }

    if (runner->callbacks.onDone)
    {
        runner->callbacks.onDone(runner->assistantMessageId, runner->promptTokens, runner->completionTokens);
    }
}

// Executes one model round and then either fails the turn, finishes it, or
// executes the requested tools and recurses into the next round. Depth is
// bounded by agentMaximumModelCalls.
void runRound(const std::shared_ptr<AgentTurnRunner> &runner, int round);

std::shared_ptr<RoundGuard> submitRound(const std::shared_ptr<AgentTurnRunner> &runner, int round);

void continueTurn(const std::shared_ptr<AgentTurnRunner> &runner,
                  int round,
                  const mathai::clients::AiStreamResult &result)
{
    runner->answer.content += result.content;
    runner->answer.reasoning += result.reasoningContent;
    if (result.promptTokens > 0)
    {
        runner->promptTokens = result.promptTokens;
    }
    if (result.completionTokens > 0)
    {
        runner->completionTokens = result.completionTokens;
    }
    runner->producedAnything = runner->producedAnything || !result.content.empty() || !result.toolCalls.empty();

    if (!result.success)
    {
        const auto code = result.busy ? std::string("busy") : std::string("ai_unavailable");
        failTurn(*runner->dao, runner->assistantMessageId, runner->answer, runner->callbacks,
                 result.errorMessage.empty() ? std::string("AI request failed") : result.errorMessage, code);
        return;
    }

    if (result.toolCalls.empty())
    {
        if (!runner->producedAnything)
        {
            failTurn(*runner->dao, runner->assistantMessageId, runner->answer, runner->callbacks,
                     "AI returned an empty answer", "ai_unavailable");
            return;
        }
        finishTurn(runner);
        return;
    }

    Json::Value roundToolCalls(Json::arrayValue);
    for (const auto &call : result.toolCalls)
    {
        Json::Value entry(Json::objectValue);
        entry["id"] = call.id;
        entry["type"] = "function";
        entry["function"]["name"] = call.name;
        entry["function"]["arguments"] = call.arguments;
        roundToolCalls.append(entry);
        runner->answer.toolCalls.append(entry);
    }

    Json::Value modelEntry(Json::objectValue);
    modelEntry["role"] = "assistant";
    modelEntry["content"] = result.content;
    modelEntry["tool_calls"] = roundToolCalls;
    // Thinking models reject the follow-up request unless the reasoning that
    // produced the tool calls is echoed back verbatim (empty is accepted).
    modelEntry["reasoning_content"] = result.reasoningContent;
    runner->messages.append(modelEntry);

    for (const auto &call : result.toolCalls)
    {
        if (runner->cancelled())
        {
            break;
        }

        const auto args = mathai::utils::parseJsonLoose(call.arguments);
        const auto startedAt = std::chrono::steady_clock::now();
        Json::Value toolResult;
        try
        {
            toolResult = tools::executeTool(call.name, runner->request.userId, args);
        }
        catch (const std::exception &exception)
        {
            Json::Value failure(Json::objectValue);
            failure["error"] = exception.what();
            toolResult = failure;
        }
        toolResult = cappedResult(toolResult);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - startedAt)
                                 .count();

        if (runner->callbacks.onToolStart)
        {
            runner->callbacks.onToolStart(call.name, args);
        }
        if (runner->callbacks.onToolDone)
        {
            runner->callbacks.onToolDone(call.name, toolResult, static_cast<long long>(elapsed));
        }

        Json::Value storedArgs(Json::objectValue);
        storedArgs["toolCallId"] = call.id;
        storedArgs["arguments"] = args;
        runner->dao->appendMessage(
            runner->conversationId,
            "tool",
            "",
            call.name,
            mathai::utils::jsonToCompactString(storedArgs),
            mathai::utils::jsonToCompactString(toolResult),
            [](long long) {},
            [](const std::string &message) {
                LOG_ERROR << "AgentService: failed to persist tool message: " << message;
            });

        Json::Value toolMessage(Json::objectValue);
        toolMessage["role"] = "tool";
        toolMessage["tool_call_id"] = call.id;
        toolMessage["content"] = mathai::utils::jsonToCompactString(toolResult);
        runner->messages.append(toolMessage);
    }

    if (runner->cancelled())
    {
        persistPartial(*runner->dao, runner->assistantMessageId, runner->answer,
                       true);
        return;
    }

    if (round + 1 >= mathai::services::agentMaximumModelCalls)
    {
        // Budget exhausted with tools still pending: keep the text produced so
        // far rather than failing the whole turn.
        finishTurn(runner);
        return;
    }

    runRound(runner, round + 1);
}

std::shared_ptr<RoundGuard> submitRound(const std::shared_ptr<AgentTurnRunner> &runner, int round)
{
    auto guard = std::make_shared<RoundGuard>();

    // The callback owns its round result; the caller only borrows it while the
    // guard is still open, and a late callback after a timeout is ignored.
    auto roundResult = std::make_shared<mathai::clients::AiStreamResult>();
    const auto settle = [runner, round, guard, roundResult](const mathai::clients::AiStreamResult &result) {
        bool expected = false;
        if (!guard->settled.compare_exchange_strong(expected, true))
        {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(guard->mutex);
            *roundResult = result;
            guard->result = result;
            guard->reported.store(true);
        }
        // Do NOT run the continuation here: this callback runs on an AiExecutor
        // worker, and continuing would submit the next round and then block in
        // runRound() waiting for it. With four workers and four concurrent
        // tool-using turns, every worker would wait on requests still queued
        // behind them, so the pool deadlocks until the 120s backstop.
        // runRound() wakes on the per-turn orchestrator thread instead and
        // drives the continuation from there.
        guard->settledCondition.notify_all();
    };

    mathai::clients::AiStreamCallbacks streamCallbacks;
    streamCallbacks.cancelled = [runner]() { return runner->cancelled(); };
    streamCallbacks.onDelta = [runner](const std::string &text) {
        if (runner->callbacks.onDelta)
        {
            runner->callbacks.onDelta(text);
        }
    };
    streamCallbacks.onFinished = settle;
    streamCallbacks.onError = settle;

    runner->service->submitStream(runner->messages, AgentService::toolSchema(), streamCallbacks);
    return guard;
}

void runRound(const std::shared_ptr<AgentTurnRunner> &runner, int round)
{
    if (runner->cancelled())
    {
        persistPartial(*runner->dao, runner->assistantMessageId, runner->answer,
                       true);
        return;
    }

    const auto guard = submitRound(runner, round);

    // Backstop for a lost callback: aiClient_ already times the transfer out at
    // 90s, so this only fires when the executor never picked the task up.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
    std::unique_lock<std::mutex> lock(guard->mutex);
    guard->settledCondition.wait_until(lock, deadline, [&guard] { return guard->reported.load(); });
    const auto settled = guard->reported.load();
    mathai::clients::AiStreamResult result;
    if (settled)
    {
        result = guard->result;
    }
    lock.unlock();

    if (!settled)
    {
        guard->settled.store(true);
        failTurn(*runner->dao, runner->assistantMessageId, runner->answer,
                 runner->callbacks, "AI request timed out", "ai_unavailable");
        return;
    }

    // The worker only recorded the result; the continuation (tool execution and
    // the next model round) runs on this orchestrator thread, which owns the
    // turn and is not part of the shared AiExecutor pool.
    continueTurn(runner, round, result);
}

// Keeps the recursion in one place for readability: a turn starts by running
// its first round on a dedicated thread. The round does not block on the model
// call itself (the AiExecutor callback drives the chain), it only waits for the
// submit to be accepted.
void servicesStartTurn(const std::shared_ptr<AgentTurnRunner> &runner)
{
    try
    {
        runRound(runner, 0);
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "AgentService: turn failed: " << exception.what();
        failTurn(*runner->dao, runner->assistantMessageId, runner->answer,
                 runner->callbacks, "agent turn failed", "internal_error");
    }
    catch (...)
    {
        LOG_ERROR << "AgentService: turn failed with an unknown error";
        failTurn(*runner->dao, runner->assistantMessageId, runner->answer,
                 runner->callbacks, "agent turn failed", "internal_error");
    }
}

void AgentService::generateTurn(const AgentTurnRequest &request,
                                long long conversationId,
                                long long excludedUserMessageId,
                                long long assistantMessageId,
                                const AgentTurnCallbacks &callbacks,
                                AgentCancelFlag cancel) const
{
    auto runner = makeTurnRunner(this, &agentDao_, request, callbacks, std::move(cancel), conversationId,
                                 assistantMessageId, excludedUserMessageId);
    std::thread([runner]() { servicesStartTurn(runner); }).detach();
}

void AgentService::submitStream(const Json::Value &messages,
                                const Json::Value &tools,
                                mathai::clients::AiStreamCallbacks callbacks) const
{
    aiClient_.generateStream(messages, tools, std::move(callbacks));
}

} // namespace mathai::services
