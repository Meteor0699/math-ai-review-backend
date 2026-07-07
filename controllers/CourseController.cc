#include "controllers/CourseController.h"

#include "utils/JsonResponse.h"

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

void CourseController::list(const drogon::HttpRequestPtr &request,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto p = mathai::utils::parsePagination(request);
    courseService_.listActive(p.page, p.pageSize, std::move(callback));
}

void CourseController::detail(const drogon::HttpRequestPtr &,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                              long long courseId)
{
    courseService_.detail(courseId, std::move(callback));
}

void CourseController::create(const drogon::HttpRequestPtr &request,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    courseService_.create(body, std::move(callback));
}

void CourseController::update(const drogon::HttpRequestPtr &request,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                              long long courseId)
{
    Json::Value body;
    if (!requireJsonBody(request, callback, body))
    {
        return;
    }
    courseService_.update(courseId, body, std::move(callback));
}

void CourseController::disable(const drogon::HttpRequestPtr &,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               long long courseId)
{
    courseService_.disable(courseId, std::move(callback));
}
