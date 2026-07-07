#include "services/CourseService.h"

#include <drogon/drogon.h>

#include <memory>

#include "utils/JsonResponse.h"

namespace mathai::services
{
namespace
{

bool hasTextField(const Json::Value &body, const char *field)
{
    return body.isMember(field) && body[field].isString() && !body[field].asString().empty();
}

void databaseError(const std::shared_ptr<CourseService::ResponseCallback> &callback,
                   const std::string &message)
{
    LOG_ERROR << "Course database error: " << message;
    (*callback)(mathai::utils::jsonResponse(500, "database error",
                                            Json::Value(Json::objectValue),
                                            drogon::k500InternalServerError));
}

} // namespace

void CourseService::listActive(int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    courseDao_.listActive(
        page, pageSize,
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

void CourseService::detail(long long id, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    courseDao_.findActiveById(
        id,
        [cb](Json::Value course) {
            if (course.isNull())
            {
                (*cb)(mathai::utils::jsonResponse(404, "course not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", course));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void CourseService::create(const Json::Value &requestBody, ResponseCallback callback) const
{
    if (!hasTextField(requestBody, "name") || !hasTextField(requestBody, "code"))
    {
        callback(mathai::utils::jsonResponse(400, "name and code are required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    courseDao_.create(
        requestBody,
        [cb](Json::Value data) {
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void CourseService::update(long long id, const Json::Value &requestBody, ResponseCallback callback) const
{
    if (!hasTextField(requestBody, "name") || !hasTextField(requestBody, "code"))
    {
        callback(mathai::utils::jsonResponse(400, "name and code are required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    courseDao_.update(
        id,
        requestBody,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "course not found",
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

void CourseService::disable(long long id, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    courseDao_.disable(
        id,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "course not found",
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
