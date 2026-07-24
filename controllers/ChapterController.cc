#include "controllers/ChapterController.h"

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

void ChapterController::listByCourse(const drogon::HttpRequestPtr &request,
                                     std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                     long long courseId)
{
    const auto p = mathai::utils::parsePagination(request);
    chapterService_.listActiveByCourse(courseId, p.page, p.pageSize, std::move(callback));
}

void ChapterController::listAll(const drogon::HttpRequestPtr &request,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    if (!mathai::utils::validOptionalIntegerParameter(request, "courseId", 1, INT64_MAX))
    {
        callback(mathai::utils::error(400, "invalid courseId", drogon::k400BadRequest));
        return;
    }
    const auto p = mathai::utils::parsePagination(request);
    chapterService_.listAll(request, p.page, p.pageSize, std::move(callback));
}

void ChapterController::detail(const drogon::HttpRequestPtr &,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               long long chapterId)
{
    chapterService_.detail(chapterId, std::move(callback));
}

void ChapterController::create(const drogon::HttpRequestPtr &request,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    chapterService_.create(body, std::move(callback));
}

void ChapterController::update(const drogon::HttpRequestPtr &request,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               long long chapterId)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    chapterService_.update(chapterId, body, std::move(callback));
}

void ChapterController::disable(const drogon::HttpRequestPtr &,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                long long chapterId)
{
    chapterService_.disable(chapterId, std::move(callback));
}
