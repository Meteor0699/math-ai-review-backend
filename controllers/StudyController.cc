#include "controllers/StudyController.h"

#include <algorithm>
#include <optional>

#include "utils/JsonResponse.h"
#include "utils/AuthContext.h"

namespace
{

int intParam(const drogon::HttpRequestPtr &request, const std::string &name, int fallback)
{
    const auto value = request->getParameter(name);
    if (value.empty())
    {
        return fallback;
    }
    try
    {
        return std::stoi(value);
    }
    catch (const std::exception &)
    {
        return fallback;
    }
}

void withClaims(const drogon::HttpRequestPtr &request,
                const std::function<void(const mathai::utils::JwtClaims &)> &handler,
                const std::function<void(const drogon::HttpResponsePtr &)> &callback)
{
    const auto auth = mathai::utils::authenticateRequest(request);
    if (auth.state != mathai::utils::AuthState::Ok)
    {
        callback(mathai::utils::authErrorResponse(auth.state));
        return;
    }
    handler(*auth.claims);
}

Json::Value bodyOrEmpty(const drogon::HttpRequestPtr &request)
{
    const auto json = request->getJsonObject();
    if (!json)
    {
        return Json::Value(Json::objectValue);
    }
    return *json;
}

} // namespace

void StudyController::getState(const drogon::HttpRequestPtr &request,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               long long questionId)
{
    withClaims(request, [&](const auto &claims) {
        studyService_.getState(claims.userId, questionId, std::move(callback));
    }, callback);
}

void StudyController::recordAttempt(const drogon::HttpRequestPtr &request,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                    long long questionId)
{
    const auto body = bodyOrEmpty(request);
    withClaims(request, [&](const auto &claims) {
        studyService_.recordAttempt(claims.userId, questionId, body, std::move(callback));
    }, callback);
}

void StudyController::addWrong(const drogon::HttpRequestPtr &request,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               long long questionId)
{
    const auto body = bodyOrEmpty(request);
    withClaims(request, [&](const auto &claims) {
        studyService_.addWrong(claims.userId, questionId, body, std::move(callback));
    }, callback);
}

void StudyController::removeWrong(const drogon::HttpRequestPtr &request,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                  long long questionId)
{
    withClaims(request, [&](const auto &claims) {
        studyService_.removeWrong(claims.userId, questionId, std::move(callback));
    }, callback);
}

void StudyController::listAttempts(const drogon::HttpRequestPtr &request,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto page = std::max(1, intParam(request, "page", 1));
    const auto pageSize = std::min(100, std::max(1, intParam(request, "pageSize", 20)));
    withClaims(request, [&](const auto &claims) {
        studyService_.listAttempts(claims.userId, page, pageSize, std::move(callback));
    }, callback);
}

void StudyController::listWrong(const drogon::HttpRequestPtr &request,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto page = std::max(1, intParam(request, "page", 1));
    const auto pageSize = std::min(100, std::max(1, intParam(request, "pageSize", 20)));
    withClaims(request, [&](const auto &claims) {
        studyService_.listWrong(claims.userId, page, pageSize, std::move(callback));
    }, callback);
}
