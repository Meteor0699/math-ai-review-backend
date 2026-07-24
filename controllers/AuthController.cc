#include "controllers/AuthController.h"

#include "utils/AuthContext.h"
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
    const auto auth = mathai::utils::authenticateRequest(request);
    if (auth.state != mathai::utils::AuthState::Ok)
    {
        callback(mathai::utils::authErrorResponse(auth.state));
        return;
    }

    Json::Value data;
    data["id"] = Json::Int64(auth.claims->userId);
    data["username"] = auth.claims->username;
    data["role"] = auth.claims->role;
    callback(mathai::utils::jsonResponse(200, "success", data));
}
