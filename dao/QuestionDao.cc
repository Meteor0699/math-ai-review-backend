﻿﻿﻿﻿#include "dao/QuestionDao.h"

#include <drogon/drogon.h>

#include <sstream>
#include <unordered_set>

#include "utils/MysqlSyncClient.h"

namespace mathai::dao
{
namespace
{

std::string quote(const std::string &value) { return mathai::utils::mysql::quote(value); }
std::string nullableString(const Json::Value &value) { return value.isNull() ? "NULL" : quote(value.asString()); }
std::string nullableInt64(const Json::Value &value) { return value.isNull() ? "NULL" : std::to_string(value.asInt64()); }

bool allowedValue(const std::string &value, const std::unordered_set<std::string> &allowed)
{
    return allowed.find(value) != allowed.end();
}

std::string questionTypeLabel(const std::string &type)
{
    if (type == "single_choice") return u8"\u5355\u9009\u9898";
    if (type == "blank") return u8"\u586b\u7a7a\u9898";
    if (type == "calculation") return u8"\u8ba1\u7b97\u9898";
    if (type == "solution") return u8"\u89e3\u7b54\u9898";
    if (type == "proof") return u8"\u8bc1\u660e\u9898";
    return type;
}

std::string difficultyLabel(const std::string &difficulty)
{
    if (difficulty == "basic") return u8"\u57fa\u7840";
    if (difficulty == "medium") return u8"\u4e2d\u7b49";
    if (difficulty == "hard") return u8"\u8f83\u96be";
    return difficulty;
}

Json::Value questionRowToJson(const mathai::utils::mysql::Row &row, bool includeAnswer)
{
    const auto questionType = row.getString("question_type");
    const auto difficulty = row.getString("difficulty");

    Json::Value item;
    item["id"] = Json::Int64(row.getInt64("id"));
    item["courseId"] = Json::Int64(row.getInt64("course_id"));
    item["chapterId"] = row.isNull("chapter_id") ? Json::Value(Json::nullValue) : Json::Value(Json::Int64(row.getInt64("chapter_id")));
    item["knowledgePointId"] = row.isNull("knowledge_point_id") ? Json::Value(Json::nullValue) : Json::Value(Json::Int64(row.getInt64("knowledge_point_id")));
    item["examPaperId"] = row.isNull("exam_paper_id") ? Json::Value(Json::nullValue) : Json::Value(Json::Int64(row.getInt64("exam_paper_id")));
    item["title"] = row.getString("title");
    item["content"] = row.getString("content");
    item["questionType"] = questionType;
    item["questionTypeLabel"] = questionTypeLabel(questionType);
    item["difficulty"] = difficulty;
    item["difficultyLabel"] = difficultyLabel(difficulty);
    item["source"] = row.getString("source");
    item["year"] = row.isNull("source_year") ? Json::Value(Json::nullValue) : Json::Value(row.getInt("source_year"));
    item["sourceYear"] = item["year"];
    item["fromExamPaper"] = row.getInt("from_exam_paper");
    item["paperQuestionNo"] = row.getString("paper_question_no");
    item["sortOrder"] = row.getInt("sort_order");
    item["status"] = row.getInt("status");

    if (includeAnswer)
    {
        item["standardAnswer"] = row.getString("standard_answer");
        item["normalExplanation"] = row.getString("normal_explanation");
    }
    return item;
}

Json::Value optionRowToJson(const mathai::utils::mysql::Row &row)
{
    Json::Value item;
    item["id"] = Json::Int64(row.getInt64("id"));
    item["questionId"] = Json::Int64(row.getInt64("question_id"));
    item["optionLabel"] = row.getString("option_label");
    item["optionContent"] = row.getString("option_content");
    item["isCorrect"] = row.getInt("is_correct");
    item["sortOrder"] = row.getInt("sort_order");
    return item;
}

std::string questionValuesSql(const Json::Value &question)
{
    std::ostringstream sql;
    sql << nullableInt64(question["courseId"]) << ", "
        << nullableInt64(question.get("chapterId", Json::Value(Json::nullValue))) << ", "
        << nullableInt64(question.get("knowledgePointId", Json::Value(Json::nullValue))) << ", "
        << nullableInt64(question.get("examPaperId", Json::Value(Json::nullValue))) << ", "
        << nullableString(question.get("title", Json::Value(Json::nullValue))) << ", "
        << quote(question["content"].asString()) << ", "
        << quote(question["questionType"].asString()) << ", "
        << quote(question.get("difficulty", "medium").asString()) << ", "
        << quote(question["standardAnswer"].asString()) << ", "
        << nullableString(question.get("normalExplanation", Json::Value(Json::nullValue))) << ", "
        << nullableString(question.get("source", Json::Value(Json::nullValue))) << ", "
        << nullableInt64(question.get("year", question.get("sourceYear", Json::Value(Json::nullValue)))) << ", "
        << question.get("fromExamPaper", 0).asInt() << ", "
        << nullableString(question.get("paperQuestionNo", Json::Value(Json::nullValue))) << ", "
        << question.get("sortOrder", 0).asInt() << ", "
        << question.get("status", 1).asInt();
    return sql.str();
}

std::string optionValuesSql(long long questionId, const Json::Value &options)
{
    std::ostringstream sql;
    for (Json::ArrayIndex i = 0; i < options.size(); ++i)
    {
        if (i > 0) sql << ", ";
        const auto &option = options[i];
        sql << "(" << questionId << ", " << quote(option["optionLabel"].asString()) << ", "
            << quote(option["optionContent"].asString()) << ", "
            << option.get("isCorrect", 0).asInt() << ", "
            << option.get("sortOrder", static_cast<int>(i + 1)).asInt() << ")";
    }
    return sql.str();
}

std::string buildStudentWhere(const drogon::HttpRequestPtr &request, const std::string &prefix = "")
{
    static const std::unordered_set<std::string> allowedTypes = {"single_choice", "blank", "calculation", "solution", "proof"};
    static const std::unordered_set<std::string> allowedDifficulties = {"basic", "medium", "hard"};
    const auto p = prefix;
    std::ostringstream where;
    where << " WHERE " << p << "status = 1";
    const auto courseId = request->getParameter("courseId");
    if (!courseId.empty()) where << " AND " << p << "course_id = " << std::stoll(courseId);
    const auto chapterId = request->getParameter("chapterId");
    if (!chapterId.empty()) where << " AND " << p << "chapter_id = " << std::stoll(chapterId);
    const auto questionType = request->getParameter("questionType");
    if (!questionType.empty() && allowedValue(questionType, allowedTypes)) where << " AND " << p << "question_type = " << quote(questionType);
    const auto difficulty = request->getParameter("difficulty");
    if (!difficulty.empty() && allowedValue(difficulty, allowedDifficulties)) where << " AND " << p << "difficulty = " << quote(difficulty);
    const auto source = request->getParameter("source");
    if (!source.empty()) where << " AND " << p << "source LIKE " << quote("%" + source + "%");
    const auto year = request->getParameter("year");
    if (!year.empty()) where << " AND " << p << "source_year = " << std::stoi(year);
    const auto keyword = request->getParameter("keyword");
    if (!keyword.empty())
    {
        const auto pattern = quote("%" + keyword + "%");
        where << " AND (" << p << "title LIKE " << pattern << " OR " << p << "content LIKE " << pattern << ")";
    }
    return where.str();
}

} // namespace

void QuestionDao::listActive(const drogon::HttpRequestPtr &request, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto whereStr = buildStudentWhere(request);
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto countResult = mathai::utils::mysql::execute("SELECT COUNT(*) AS total FROM question" + whereStr);
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, course_id, chapter_id, knowledge_point_id, exam_paper_id, title, content, question_type, difficulty, "
            "standard_answer, normal_explanation, source, source_year, from_exam_paper, paper_question_no, sort_order, status "
            "FROM question" + whereStr + " ORDER BY sort_order ASC, id ASC LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset));
        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows) items.append(questionRowToJson(row, false));
        Json::Value data;
        data["items"] = items;
        data["total"] = total;
        onSuccess(data);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::listAdmin(const drogon::HttpRequestPtr &request, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto whereStr = buildStudentWhere(request, "q.");
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto countResult = mathai::utils::mysql::execute("SELECT COUNT(*) AS total FROM question q" + whereStr);
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto result = mathai::utils::mysql::execute(
            "SELECT q.id, q.course_id, q.chapter_id, q.knowledge_point_id, q.exam_paper_id, q.title, q.content, q.question_type, q.difficulty, "
            "q.standard_answer, q.normal_explanation, q.source, q.source_year, q.from_exam_paper, q.paper_question_no, q.sort_order, q.status, "
            "c.name AS course_name, ch.title AS chapter_name FROM question q "
            "LEFT JOIN course c ON q.course_id = c.id LEFT JOIN chapter ch ON q.chapter_id = ch.id" +
            whereStr + " ORDER BY q.sort_order ASC, q.id ASC LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset));
        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows)
        {
            auto item = questionRowToJson(row, false);
            item["courseName"] = row.getString("course_name");
            item["chapterName"] = row.getString("chapter_name");
            items.append(item);
        }
        Json::Value data;
        data["items"] = items;
        data["total"] = total;
        onSuccess(data);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::findActiveById(long long id, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, course_id, chapter_id, knowledge_point_id, exam_paper_id, title, content, question_type, difficulty, "
            "standard_answer, normal_explanation, source, source_year, from_exam_paper, paper_question_no, sort_order, status "
            "FROM question WHERE id = " + std::to_string(id) + " AND status = 1 LIMIT 1");
        if (result.rows.empty()) { onSuccess(Json::Value(Json::nullValue)); return; }
        onSuccess(questionRowToJson(result.rows[0], true));
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::listOptions(long long questionId, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, question_id, option_label, option_content, is_correct, sort_order FROM question_option WHERE question_id = " +
            std::to_string(questionId) + " ORDER BY sort_order ASC, id ASC");
        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows) items.append(optionRowToJson(row));
        onSuccess(items);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::create(const Json::Value &question, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "INSERT INTO question (course_id, chapter_id, knowledge_point_id, exam_paper_id, title, content, question_type, difficulty, "
            "standard_answer, normal_explanation, source, source_year, from_exam_paper, paper_question_no, sort_order, status) VALUES (" +
            questionValuesSql(question) + ")");
        Json::Value data;
        data["id"] = Json::UInt64(result.insertId);
        onSuccess(data);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::update(long long id, const Json::Value &question, CountCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        std::ostringstream sql;
        sql << "UPDATE question SET course_id = " << nullableInt64(question["courseId"])
            << ", chapter_id = " << nullableInt64(question.get("chapterId", Json::Value(Json::nullValue)))
            << ", knowledge_point_id = " << nullableInt64(question.get("knowledgePointId", Json::Value(Json::nullValue)))
            << ", exam_paper_id = " << nullableInt64(question.get("examPaperId", Json::Value(Json::nullValue)))
            << ", title = " << nullableString(question.get("title", Json::Value(Json::nullValue)))
            << ", content = " << quote(question["content"].asString())
            << ", question_type = " << quote(question["questionType"].asString())
            << ", difficulty = " << quote(question.get("difficulty", "medium").asString())
            << ", standard_answer = " << quote(question["standardAnswer"].asString())
            << ", normal_explanation = " << nullableString(question.get("normalExplanation", Json::Value(Json::nullValue)))
            << ", source = " << nullableString(question.get("source", Json::Value(Json::nullValue)))
            << ", source_year = " << nullableInt64(question.get("year", question.get("sourceYear", Json::Value(Json::nullValue))))
            << ", from_exam_paper = " << question.get("fromExamPaper", 0).asInt()
            << ", paper_question_no = " << nullableString(question.get("paperQuestionNo", Json::Value(Json::nullValue)))
            << ", sort_order = " << question.get("sortOrder", 0).asInt()
            << ", status = " << question.get("status", 1).asInt()
            << " WHERE id = " << id;
        const auto result = mathai::utils::mysql::execute(sql.str());
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::disable(long long id, CountCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute("UPDATE question SET status = 0 WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::addOptions(long long questionId, const Json::Value &options, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        if (!options.isArray() || options.empty())
        {
            Json::Value data;
            data["created"] = 0;
            onSuccess(data);
            return;
        }
        const auto result = mathai::utils::mysql::execute(
            "INSERT INTO question_option (question_id, option_label, option_content, is_correct, sort_order) VALUES " +
            optionValuesSql(questionId, options));
        Json::Value data;
        data["created"] = Json::UInt64(result.affectedRows);
        onSuccess(data);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::replaceOptions(long long questionId, const Json::Value &options, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        mathai::utils::mysql::execute("DELETE FROM question_option WHERE question_id = " + std::to_string(questionId));
        addOptions(questionId, options, onSuccess, onError);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

void QuestionDao::deleteOption(long long optionId, CountCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute("DELETE FROM question_option WHERE id = " + std::to_string(optionId));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception) { onError(exception.what()); }
}

} // namespace mathai::dao
