#include "controllers/UserController.h"

#include <ctime>
#include <memory>

#include "utils/JsonResponse.h"
#include "utils/PasswordUtil.h"

namespace
{

bool hasTextField(const Json::Value &body, const char *field)
{
    return body.isMember(field) && body[field].isString() && !body[field].asString().empty();
}

bool validRole(const Json::Value &body)
{
    const auto role = body.get("role", "student").asString();
    return role == "student" || role == "admin";
}

std::string makeSalt(const std::string &seed)
{
    return mathai::utils::sha256Hex(seed + ":" + std::to_string(std::time(nullptr))).substr(0, 16);
}

bool duplicateKeyError(const std::string &message)
{
    return message.find("Duplicate entry") != std::string::npos;
}

drogon::HttpResponsePtr databaseError(const std::string &message)
{
    LOG_ERROR << "User database error: " << message;
    if (duplicateKeyError(message))
    {
        return mathai::utils::jsonResponse(409, "username or student number already exists",
                                           Json::Value(Json::objectValue),
                                           drogon::k409Conflict);
    }
    return mathai::utils::jsonResponse(500, "database error",
                                       Json::Value(Json::objectValue),
                                       drogon::k500InternalServerError);
}

} // namespace

void UserController::list(const drogon::HttpRequestPtr &request,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto pagination = mathai::utils::parsePagination(request);
    auto cb = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(callback));
    userDao_.list(
        request->getParameter("keyword"),
        request->getParameter("role"),
        pagination.page,
        pagination.pageSize,
        [cb, pagination](Json::Value result) {
            Json::Value data;
            data["items"] = result["items"];
            data["total"] = result["total"];
            data["page"] = pagination.page;
            data["pageSize"] = pagination.pageSize;
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            (*cb)(databaseError(message));
        });
}

void UserController::create(const drogon::HttpRequestPtr &request,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto json = request->getJsonObject();
    if (!json || !hasTextField(*json, "username") || !hasTextField(*json, "password") || !validRole(*json))
    {
        callback(mathai::utils::jsonResponse(400, "invalid user body",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    const auto username = (*json)["username"].asString();
    const auto password = (*json)["password"].asString();
    if (username.size() < 2 || username.size() > 64 || password.size() < 6 || password.size() > 64)
    {
        callback(mathai::utils::jsonResponse(400, "invalid username or password length",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    Json::Value user = *json;
    user["passwordHash"] = mathai::utils::makePasswordHash(password, makeSalt(username));
    user["status"] = user.get("status", 1);

    auto cb = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(callback));
    userDao_.create(
        user,
        [cb](Json::Value data) {
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            (*cb)(databaseError(message));
        });
}

void UserController::update(const drogon::HttpRequestPtr &request,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                            long long userId)
{
    const auto json = request->getJsonObject();
    if (!json || !validRole(*json))
    {
        callback(mathai::utils::jsonResponse(400, "invalid user body",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(callback));
    userDao_.update(
        userId,
        *json,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "user not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            (*cb)(databaseError(message));
        });
}

void UserController::disable(const drogon::HttpRequestPtr &,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             long long userId)
{
    auto cb = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(callback));
    userDao_.disable(
        userId,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "user not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            (*cb)(databaseError(message));
        });
}

void UserController::resetPassword(const drogon::HttpRequestPtr &request,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                   long long userId)
{
    const auto json = request->getJsonObject();
    if (!json || !hasTextField(*json, "password"))
    {
        callback(mathai::utils::jsonResponse(400, "password is required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    const auto password = (*json)["password"].asString();
    if (password.size() < 6 || password.size() > 64)
    {
        callback(mathai::utils::jsonResponse(400, "invalid password length",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(callback));
    userDao_.updatePassword(
        userId,
        mathai::utils::makePasswordHash(password, makeSalt(std::to_string(userId))),
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "user not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            (*cb)(databaseError(message));
        });
}
