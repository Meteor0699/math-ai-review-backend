#include "services/AuthService.h"

#include "utils/JwtUtil.h"
#include "utils/JsonResponse.h"
#include "utils/PasswordUtil.h"

#include <memory>

namespace mathai::services
{
namespace
{

bool hasTextField(const Json::Value &body, const char *field)
{
    return body.isMember(field) && body[field].isString() && !body[field].asString().empty();
}

bool duplicateKeyError(const std::string &message)
{
    return message.find("Duplicate entry") != std::string::npos;
}

} // namespace

void AuthService::login(const std::string &username,
                        const std::string &password,
                        ResponseCallback callback) const
{
    if (username.empty() || password.empty())
    {
        callback(mathai::utils::jsonResponse(400, "username and password are required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto sharedCallback = std::make_shared<ResponseCallback>(std::move(callback));
    userDao_.findByUsername(
        username,
        [this, password, sharedCallback](std::optional<mathai::models::User> user) {
            if (!user || user->status != 1 ||
                !mathai::utils::verifyPassword(password, user->passwordHash))
            {
                (*sharedCallback)(mathai::utils::jsonResponse(401, "invalid username or password",
                                                              Json::Value(Json::objectValue),
                                                              drogon::k401Unauthorized));
                return;
            }

            std::string upgradedPasswordHash;
            try
            {
                if (mathai::utils::passwordNeedsRehash(user->passwordHash))
                {
                    upgradedPasswordHash = mathai::utils::makePasswordHash(password);
                }
            }
            catch (const std::exception &exception)
            {
                LOG_ERROR << "Password upgrade error: " << exception.what();
                (*sharedCallback)(mathai::utils::jsonResponse(500, "authentication service unavailable",
                                                              Json::Value(Json::objectValue),
                                                              drogon::k500InternalServerError));
                return;
            }

            userDao_.recordSuccessfulLogin(
                user->id,
                user->authVersion,
                upgradedPasswordHash,
                [sharedCallback, user](std::uint64_t affectedRows) {
                    if (affectedRows == 0)
                    {
                        (*sharedCallback)(mathai::utils::jsonResponse(401, "invalid username or password",
                                                                      Json::Value(Json::objectValue),
                                                                      drogon::k401Unauthorized));
                        return;
                    }
                    Json::Value data;
                    data["token"] = mathai::utils::createJwt(user->id, user->username, user->role, user->authVersion);
                    data["user"]["id"] = Json::Int64(user->id);
                    data["user"]["username"] = user->username;
                    data["user"]["realName"] = user->realName;
                    data["user"]["role"] = user->role;
                    (*sharedCallback)(mathai::utils::jsonResponse(200, "success", data));
                },
                [sharedCallback](const std::string &errorMessage) {
                    LOG_ERROR << "Login audit database error: " << errorMessage;
                    (*sharedCallback)(mathai::utils::jsonResponse(500, "database error",
                                                                  Json::Value(Json::objectValue),
                                                                  drogon::k500InternalServerError));
                });
        },
        [sharedCallback](const std::string &errorMessage) {
            LOG_ERROR << "Login database error: " << errorMessage;
            (*sharedCallback)(mathai::utils::jsonResponse(500, "database error",
                                                          Json::Value(Json::objectValue),
                                                          drogon::k500InternalServerError));
        });
}

void AuthService::registerUser(const Json::Value &requestBody,
                               ResponseCallback callback) const
{
    if (!hasTextField(requestBody, "username") || !hasTextField(requestBody, "password"))
    {
        callback(mathai::utils::jsonResponse(400, "username and password are required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    const auto username = requestBody["username"].asString();
    const auto password = requestBody["password"].asString();
    if (username.size() < 2 || username.size() > 64 || password.size() < 6 || password.size() > 64)
    {
        callback(mathai::utils::jsonResponse(400, "invalid username or password length",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    if ((requestBody.isMember("realName") && !requestBody["realName"].isString()) ||
        (requestBody.isMember("studentNo") && !requestBody["studentNo"].isString()) ||
        requestBody.get("realName", "").asString().size() > 64 ||
        requestBody.get("studentNo", "").asString().size() > 64)
    {
        callback(mathai::utils::jsonResponse(400, "invalid profile fields",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    Json::Value user;
    user["username"] = username;
    try
    {
        user["passwordHash"] = mathai::utils::makePasswordHash(password);
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Password hashing error: " << exception.what();
        callback(mathai::utils::jsonResponse(500, "authentication service unavailable",
                                             Json::Value(Json::objectValue),
                                             drogon::k500InternalServerError));
        return;
    }
    user["realName"] = requestBody.get("realName", "");
    user["studentNo"] = requestBody.get("studentNo", "");
    user["role"] = "student";
    user["status"] = 1;

    auto sharedCallback = std::make_shared<ResponseCallback>(std::move(callback));
    userDao_.create(
        user,
        [sharedCallback, username, realName = user["realName"].asString()](Json::Value created) {
            const auto userId = created["id"].asInt64();
            Json::Value data;
            data["token"] = mathai::utils::createJwt(userId, username, "student", 1);
            data["user"]["id"] = Json::Int64(userId);
            data["user"]["username"] = username;
            data["user"]["realName"] = realName;
            data["user"]["role"] = "student";
            (*sharedCallback)(mathai::utils::jsonResponse(200, "success", data));
        },
        [sharedCallback](const std::string &errorMessage) {
            LOG_ERROR << "Register database error: " << errorMessage;
            if (duplicateKeyError(errorMessage))
            {
                (*sharedCallback)(mathai::utils::jsonResponse(409, "username or student number already exists",
                                                              Json::Value(Json::objectValue),
                                                              drogon::k409Conflict));
                return;
            }
            (*sharedCallback)(mathai::utils::jsonResponse(500, "database error",
                                                          Json::Value(Json::objectValue),
                                                          drogon::k500InternalServerError));
        });
}

} // namespace mathai::services
