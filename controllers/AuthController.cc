#include "controllers/AuthController.h"

#include "utils/JwtUtil.h"
#include "utils/JsonResponse.h"

void AuthController::login(const drogon::HttpRequestPtr &request,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto json = request->getJsonObject();
    if (!json || !(*json)["username"].isString() || !(*json)["password"].isString())
    {
        callback(mathai::utils::jsonResponse(400, "invalid request body",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    authService_.login((*json)["username"].asString(),
                       (*json)["password"].asString(),
                       std::move(callback));
}

void AuthController::registerUser(const drogon::HttpRequestPtr &request,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto json = request->getJsonObject();
    if (!json)
    {
        callback(mathai::utils::jsonResponse(400, "invalid request body",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    authService_.registerUser(*json, std::move(callback));
}

void AuthController::me(const drogon::HttpRequestPtr &request,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto authHeader = request->getHeader("Authorization");
    const std::string prefix = "Bearer ";
    if (authHeader.rfind(prefix, 0) != 0)
    {
        callback(mathai::utils::jsonResponse(401, "missing token",
                                             Json::Value(Json::objectValue),
                                             drogon::k401Unauthorized));
        return;
    }

    const auto claims = mathai::utils::verifyJwt(authHeader.substr(prefix.size()));
    if (!claims)
    {
        callback(mathai::utils::jsonResponse(401, "invalid token",
                                             Json::Value(Json::objectValue),
                                             drogon::k401Unauthorized));
        return;
    }

    Json::Value data;
    data["id"] = Json::Int64(claims->userId);
    data["username"] = claims->username;
    data["role"] = claims->role;
    callback(mathai::utils::jsonResponse(200, "success", data));
}
