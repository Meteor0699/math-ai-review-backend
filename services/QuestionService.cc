#include "services/QuestionService.h"

#include <drogon/drogon.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "utils/JsonResponse.h"

namespace mathai::services
{
namespace
{

bool allowedValue(const Json::Value &body,
                  const char *field,
                  const std::unordered_set<std::string> &allowed,
                  bool required)
{
    if (!body.isMember(field) || body[field].isNull())
    {
        return !required;
    }
    return body[field].isString() && allowed.find(body[field].asString()) != allowed.end();
}

bool validQuestionBody(const Json::Value &body)
{
    static const std::unordered_set<std::string> allowedTypes = {
        "single_choice", "blank", "calculation", "solution", "proof"};
    static const std::unordered_set<std::string> allowedDifficulties = {
        "basic", "medium", "hard"};

    return body.isMember("courseId") && body["courseId"].isIntegral() &&
           body.isMember("content") && body["content"].isString() && !body["content"].asString().empty() &&
           body.isMember("standardAnswer") && body["standardAnswer"].isString() &&
           allowedValue(body, "questionType", allowedTypes, true) &&
           allowedValue(body, "difficulty", allowedDifficulties, false);
}

bool validOptionsBody(const Json::Value &body)
{
    if (!body.isMember("options") || !body["options"].isArray())
    {
        return false;
    }

    const auto &options = body["options"];
    for (const auto &option : options)
    {
        if (!option.isMember("optionLabel") || !option["optionLabel"].isString() ||
            option["optionLabel"].asString().empty() ||
            !option.isMember("optionContent") || !option["optionContent"].isString())
        {
            return false;
        }
    }
    return true;
}

Json::Value normalizeQuestionBody(const Json::Value &body)
{
    Json::Value normalized = body;

    static const std::unordered_map<std::string, std::string> typeMap = {
        {"fill_blank", "blank"},
        {"单选题", "single_choice"},
        {"选择题", "single_choice"},
        {"填空题", "blank"},
        {"计算题", "calculation"},
        {"解答题", "solution"},
        {"证明题", "proof"}};
    static const std::unordered_map<std::string, std::string> difficultyMap = {
        {"基础", "basic"},
        {"中等", "medium"},
        {"较难", "hard"},
        {"困难", "hard"}};

    if (normalized.isMember("questionType") && normalized["questionType"].isString())
    {
        const auto it = typeMap.find(normalized["questionType"].asString());
        if (it != typeMap.end())
        {
            normalized["questionType"] = it->second;
        }
    }

    if (normalized.isMember("difficulty") && normalized["difficulty"].isString())
    {
        const auto it = difficultyMap.find(normalized["difficulty"].asString());
        if (it != difficultyMap.end())
        {
            normalized["difficulty"] = it->second;
        }
    }

    if (!normalized.isMember("analysis") && normalized.isMember("normalExplanation"))
    {
        normalized["analysis"] = normalized["normalExplanation"];
    }

    if (!normalized.isMember("year") && normalized.isMember("sourceYear"))
    {
        if (normalized["sourceYear"].isIntegral() || normalized["sourceYear"].isNull())
        {
            normalized["year"] = normalized["sourceYear"];
        }
        else if (normalized["sourceYear"].isString() && !normalized["sourceYear"].asString().empty())
        {
            try
            {
                normalized["year"] = std::stoi(normalized["sourceYear"].asString());
            }
            catch (...)
            {
                normalized["year"] = Json::Value(Json::nullValue);
            }
        }
    }

    if (!normalized.isMember("isFromPaper") && normalized.isMember("fromExamPaper"))
    {
        normalized["isFromPaper"] = normalized["fromExamPaper"].asBool() ? 1 : 0;
    }

    return normalized;
}

void databaseError(const std::shared_ptr<QuestionService::ResponseCallback> &callback,
                   const std::string &message)
{
    LOG_ERROR << "Question database error: " << message;
    (*callback)(mathai::utils::jsonResponse(500, "database error",
                                            Json::Value(Json::objectValue),
                                            drogon::k500InternalServerError));
}

} // namespace

void QuestionService::list(const drogon::HttpRequestPtr &request, int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.listActive(
        request,
        page,
        pageSize,
        [cb, page, pageSize](Json::Value result) {
            Json::Value data;
            data["items"] = result["items"];
            data["total"] = result["total"];
            data["page"] = page;
            data["pageSize"] = pageSize;
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void QuestionService::listAdmin(const drogon::HttpRequestPtr &request, int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.listAdmin(
        request,
        page,
        pageSize,
        [cb, page, pageSize](Json::Value result) {
            Json::Value data;
            data["items"] = result["items"];
            data["total"] = result["total"];
            data["page"] = page;
            data["pageSize"] = pageSize;
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void QuestionService::detail(long long id, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.findActiveById(
        id,
        [this, id, cb](Json::Value question) {
            if (question.isNull())
            {
                (*cb)(mathai::utils::jsonResponse(404, "question not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }

            questionDao_.listOptions(
                id,
                [cb, question](Json::Value options) mutable {
                    question["options"] = options;
                    (*cb)(mathai::utils::jsonResponse(200, "success", question));
                },
                [cb](const std::string &message) {
                    databaseError(cb, message);
                });
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void QuestionService::create(const Json::Value &requestBody, ResponseCallback callback) const
{
    const auto normalizedBody = normalizeQuestionBody(requestBody);
    if (!validQuestionBody(normalizedBody))
    {
        callback(mathai::utils::jsonResponse(400, "invalid question body",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.create(
        normalizedBody,
        [cb](Json::Value data) {
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void QuestionService::update(long long id, const Json::Value &requestBody, ResponseCallback callback) const
{
    const auto normalizedBody = normalizeQuestionBody(requestBody);
    if (!validQuestionBody(normalizedBody))
    {
        callback(mathai::utils::jsonResponse(400, "invalid question body",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.update(
        id,
        normalizedBody,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "question not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void QuestionService::disable(long long id, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.disable(
        id,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "question not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void QuestionService::addOptions(long long questionId, const Json::Value &requestBody, ResponseCallback callback) const
{
    if (!validOptionsBody(requestBody))
    {
        callback(mathai::utils::jsonResponse(400, "options array is required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.addOptions(
        questionId,
        requestBody["options"],
        [cb](Json::Value data) {
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void QuestionService::replaceOptions(long long questionId, const Json::Value &requestBody, ResponseCallback callback) const
{
    if (!validOptionsBody(requestBody))
    {
        callback(mathai::utils::jsonResponse(400, "options array is required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.replaceOptions(
        questionId,
        requestBody["options"],
        [cb](Json::Value data) {
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void QuestionService::deleteOption(long long optionId, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    questionDao_.deleteOption(
        optionId,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "option not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

} // namespace mathai::services
