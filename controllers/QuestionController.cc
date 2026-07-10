#include "controllers/QuestionController.h"

#include "utils/JsonResponse.h"
#include "utils/RequestValidation.h"

namespace
{

bool requireJsonBody(const drogon::HttpRequestPtr &request,
                     std::function<void(const drogon::HttpResponsePtr &)> &callback,
                     Json::Value &body)
{
    const auto json = request->getJsonObject();
    if (!json)
    {
        callback(mathai::utils::jsonResponse(400, "invalid request body",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return false;
    }
    body = *json;
    return true;
}

bool validQuestionFilters(const drogon::HttpRequestPtr &request)
{
    return mathai::utils::validOptionalIntegerParameter(request, "courseId", 1, INT64_MAX) &&
           mathai::utils::validOptionalIntegerParameter(request, "chapterId", 1, INT64_MAX) &&
           mathai::utils::validOptionalIntegerParameter(request, "year", 1900, 2155);
}

} // namespace

void QuestionController::list(const drogon::HttpRequestPtr &request,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    if (!validQuestionFilters(request))
    {
        callback(mathai::utils::error(400, "invalid question filter", drogon::k400BadRequest));
        return;
    }
    const auto p = mathai::utils::parsePagination(request);
    questionService_.list(request, p.page, p.pageSize, std::move(callback));
}

void QuestionController::listAdmin(const drogon::HttpRequestPtr &request,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    if (!validQuestionFilters(request))
    {
        callback(mathai::utils::error(400, "invalid question filter", drogon::k400BadRequest));
        return;
    }
    const auto p = mathai::utils::parsePagination(request);
    questionService_.listAdmin(request, p.page, p.pageSize, std::move(callback));
}

void QuestionController::detail(const drogon::HttpRequestPtr &,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                long long questionId)
{
    questionService_.detail(questionId, std::move(callback));
}

void QuestionController::create(const drogon::HttpRequestPtr &request,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    questionService_.create(body, std::move(callback));
}

void QuestionController::update(const drogon::HttpRequestPtr &request,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                long long questionId)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    questionService_.update(questionId, body, std::move(callback));
}

void QuestionController::disable(const drogon::HttpRequestPtr &,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 long long questionId)
{
    questionService_.disable(questionId, std::move(callback));
}

void QuestionController::addOptions(const drogon::HttpRequestPtr &request,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                    long long questionId)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    questionService_.addOptions(questionId, body, std::move(callback));
}

void QuestionController::replaceOptions(const drogon::HttpRequestPtr &request,
                                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                        long long questionId)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    questionService_.replaceOptions(questionId, body, std::move(callback));
}

void QuestionController::deleteOption(const drogon::HttpRequestPtr &,
                                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                      long long,
                                      long long optionId)
{
    questionService_.deleteOption(optionId, std::move(callback));
}
