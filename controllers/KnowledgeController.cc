#include "controllers/KnowledgeController.h"

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

} // namespace

void KnowledgeController::listByChapter(const drogon::HttpRequestPtr &request,
                                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                        long long chapterId)
{
    const auto p = mathai::utils::parsePagination(request);
    knowledgeService_.listActiveByChapter(chapterId, p.page, p.pageSize, std::move(callback));
}

void KnowledgeController::listAll(const drogon::HttpRequestPtr &request,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    if (!mathai::utils::validOptionalIntegerParameter(request, "courseId", 1, INT64_MAX) ||
        !mathai::utils::validOptionalIntegerParameter(request, "chapterId", 1, INT64_MAX))
    {
        callback(mathai::utils::error(400, "invalid courseId or chapterId", drogon::k400BadRequest));
        return;
    }
    const auto p = mathai::utils::parsePagination(request);
    knowledgeService_.listAll(request, p.page, p.pageSize, std::move(callback));
}

void KnowledgeController::create(const drogon::HttpRequestPtr &request,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    knowledgeService_.create(body, std::move(callback));
}

void KnowledgeController::update(const drogon::HttpRequestPtr &request,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 long long knowledgeId)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    knowledgeService_.update(knowledgeId, body, std::move(callback));
}

void KnowledgeController::disable(const drogon::HttpRequestPtr &,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                  long long knowledgeId)
{
    knowledgeService_.disable(knowledgeId, std::move(callback));
}
