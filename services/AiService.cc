#include "services/AiService.h"

#include <drogon/drogon.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>
#include <string>

#include "utils/JsonResponse.h"

namespace mathai::services
{
namespace
{

void databaseError(const std::shared_ptr<AiService::ResponseCallback> &callback,
                   const std::string &message)
{
    LOG_ERROR << "AI explanation database error: " << message;
    (*callback)(mathai::utils::jsonResponse(500, "database error",
                                            Json::Value(Json::objectValue),
                                            drogon::k500InternalServerError));
}

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

std::string safeText(const Json::Value &value, const char *field)
{
    if (!value.isMember(field) || value[field].isNull())
    {
        return "";
    }
    return value[field].asString();
}

} // namespace

void AiService::explainQuestion(long long questionId, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));

    aiExplanationDao_.findExisting(
        questionId,
        [this, questionId, cb](Json::Value existing) {
            if (!existing.isNull())
            {
                (*cb)(mathai::utils::jsonResponse(200, "success", existing));
                return;
            }

            aiExplanationDao_.findQuestionContext(
                questionId,
                [this, questionId, cb](Json::Value questionContext) {
                    if (questionContext.isNull())
                    {
                        (*cb)(mathai::utils::jsonResponse(404, "question not found",
                                                          Json::Value(Json::objectValue),
                                                          drogon::k404NotFound));
                        return;
                    }

                    const auto prompt = buildPrompt(questionContext);
                    aiClient_.generateExplanation(
                        prompt,
                        [this, questionId, prompt, cb](mathai::clients::AiResult result) {
                            if (!result.success)
                            {
                                Json::Value data;
                                data["questionId"] = Json::Int64(questionId);
                                data["error"] = result.errorMessage;
                                (*cb)(mathai::utils::jsonResponse(502, "ai generation failed", data,
                                                                  drogon::k502BadGateway));
                                return;
                            }

                            aiExplanationDao_.saveSuccess(
                                questionId,
                                result.modelName,
                                prompt,
                                result.content,
                                [cb](Json::Value saved) {
                                    (*cb)(mathai::utils::jsonResponse(200, "success", saved));
                                },
                                [cb](const std::string &message) {
                                    databaseError(cb, message);
                                });
                        });
                },
                [cb](const std::string &message) {
                    databaseError(cb, message);
                });
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void AiService::followUpQuestion(long long questionId,
                                 const Json::Value &requestBody,
                                 ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    if (!requestBody.isMember("question") || !requestBody["question"].isString())
    {
        (*cb)(mathai::utils::jsonResponse(400, "question is required",
                                          Json::Value(Json::objectValue),
                                          drogon::k400BadRequest));
        return;
    }

    const auto studentQuestion = trim(requestBody["question"].asString());
    if (studentQuestion.size() < 2 || studentQuestion.size() > 500)
    {
        (*cb)(mathai::utils::jsonResponse(400, "question length must be between 2 and 500 characters",
                                          Json::Value(Json::objectValue),
                                          drogon::k400BadRequest));
        return;
    }

    aiExplanationDao_.findQuestionContext(
        questionId,
        [this, questionId, studentQuestion, cb](Json::Value questionContext) {
            if (questionContext.isNull())
            {
                (*cb)(mathai::utils::jsonResponse(404, "question not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }

            aiExplanationDao_.findExisting(
                questionId,
                [this, questionId, questionContext, studentQuestion, cb](Json::Value existing) {
                    const auto prompt = buildFollowUpPrompt(questionContext, existing, studentQuestion);
                    aiClient_.generateExplanation(
                        prompt,
                        [questionId, studentQuestion, cb](mathai::clients::AiResult result) {
                            if (!result.success)
                            {
                                Json::Value data;
                                data["questionId"] = Json::Int64(questionId);
                                data["error"] = result.errorMessage;
                                (*cb)(mathai::utils::jsonResponse(502, "ai generation failed", data,
                                                                  drogon::k502BadGateway));
                                return;
                            }

                            Json::Value data;
                            data["questionId"] = Json::Int64(questionId);
                            data["question"] = studentQuestion;
                            data["answer"] = result.content;
                            data["modelName"] = result.modelName;
                            (*cb)(mathai::utils::jsonResponse(200, "success", data));
                        });
                },
                [cb](const std::string &message) {
                    databaseError(cb, message);
                });
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

std::string AiService::buildPrompt(const Json::Value &questionContext) const
{
    std::ostringstream prompt;
    prompt << "你是一名大学数学课程复习辅导老师，请针对下面这道题进行讲解。\n"
           << "要求：\n"
           << "1. 先说明本题考查的知识点。\n"
           << "2. 再给出清晰的解题思路。\n"
           << "3. 然后分步骤讲解解题过程。\n"
           << "4. 最后指出易错点和同类题复习建议。\n"
           << "5. 语言要适合大学生期末复习，通俗、准确、不要编造题目条件。\n"
           << "6. 数学公式请优先使用 LaTeX，行内公式用 \\(...\\)，块级公式用 \\[...\\]。\n\n"
           << "课程：" << safeText(questionContext, "courseName") << "\n"
           << "章节：" << safeText(questionContext, "chapterName") << "\n"
           << "题型：" << questionTypeLabel(safeText(questionContext, "questionType")) << "\n"
           << "题目：" << safeText(questionContext, "content") << "\n"
           << "标准答案：" << safeText(questionContext, "standardAnswer") << "\n"
           << "已有解析：" << safeText(questionContext, "analysis") << "\n\n"
           << "输出格式：\n"
           << "一、本题考点\n"
           << "二、解题思路\n"
           << "三、详细步骤\n"
           << "四、易错提醒\n"
           << "五、同类题建议";
    return prompt.str();
}

std::string AiService::buildFollowUpPrompt(const Json::Value &questionContext,
                                           const Json::Value &existingExplanation,
                                           const std::string &studentQuestion) const
{
    std::ostringstream prompt;
    prompt << "你是一名大学数学课程复习辅导老师，现在学生正在针对一道已有题目继续追问。\n"
           << "请先在心里判断学生追问是否与数学学习、当前题目、课程章节、解题步骤、公式推导、易错点或复习方法相关。\n"
           << "如果相关：直接回答学生追问，回答要短而清楚，优先围绕追问展开，不要重新完整讲一遍题目。\n"
           << "如果不相关：不要回答无关内容，只回复一句：这个问题和当前数学学习无关，我只能回答与本题或数学复习相关的问题。\n"
           << "不要编造题目中不存在的条件；如果信息不足，要说明依据当前题目只能推到哪里。\n"
           << "数学公式请使用 LaTeX，行内公式用 \\(...\\)，块级公式用 \\[...\\]。\n\n"
           << "课程：" << safeText(questionContext, "courseName") << "\n"
           << "章节：" << safeText(questionContext, "chapterName") << "\n"
           << "题型：" << questionTypeLabel(safeText(questionContext, "questionType")) << "\n"
           << "题目：" << safeText(questionContext, "content") << "\n"
           << "标准答案：" << safeText(questionContext, "standardAnswer") << "\n"
           << "普通解析：" << safeText(questionContext, "analysis") << "\n"
           << "已有 AI 讲解：" << safeText(existingExplanation, "explanation") << "\n\n"
           << "学生追问：" << studentQuestion << "\n\n"
           << "请按上面的相关性规则直接输出给学生看的回答。";
    return prompt.str();
}

std::string AiService::questionTypeLabel(const std::string &type) const
{
    if (type == "single_choice")
    {
        return "单选题";
    }
    if (type == "blank")
    {
        return "填空题";
    }
    if (type == "calculation")
    {
        return "计算题";
    }
    if (type == "solution")
    {
        return "解答题";
    }
    if (type == "proof")
    {
        return "证明题";
    }
    return type;
}

} // namespace mathai::services
