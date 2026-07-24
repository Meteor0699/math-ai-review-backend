#include "utils/AuthContext.h"

#include <drogon/drogon.h>

#include "utils/JsonResponse.h"
#include "utils/MysqlSyncClient.h"

namespace mathai::utils
{
namespace
{

constexpr const char *claimsAttributeKey = "mathai.auth.claims";

} // namespace

AuthResult authenticateRequest(const drogon::HttpRequestPtr &request)
{
    const auto attributes = request->attributes();
    if (attributes->find(claimsAttributeKey))
    {
        return {AuthState::Ok, attributes->get<JwtClaims>(claimsAttributeKey)};
    }

    const auto authHeader = request->getHeader("Authorization");
    constexpr std::string_view prefix{"Bearer "};
    if (authHeader.rfind(prefix.data(), 0) != 0 || authHeader.size() == prefix.size())
    {
        return {AuthState::Missing, std::nullopt};
    }

    const auto claims = verifyJwt(authHeader.substr(prefix.size()));
    if (!claims)
    {
        return {AuthState::Invalid, std::nullopt};
    }

    try
    {
        const auto result = mysql::execute(
            "SELECT role, status, auth_version FROM user WHERE id = " +
            std::to_string(claims->userId) + " LIMIT 1");
        if (result.rows.empty())
        {
            return {AuthState::Invalid, std::nullopt};
        }

        const auto &row = result.rows.front();
        if (row.getInt("status") != 1 ||
            row.getString("role") != claims->role ||
            row.getInt("auth_version", 0) != claims->authVersion)
        {
            return {AuthState::Invalid, std::nullopt};
        }

        attributes->insert(claimsAttributeKey, *claims);
        return {AuthState::Ok, claims};
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Authentication database error: " << exception.what();
        return {AuthState::DatabaseError, std::nullopt};
    }
}

drogon::HttpResponsePtr authErrorResponse(AuthState state)
{
    if (state == AuthState::DatabaseError)
    {
        return jsonResponse(500, "authentication service unavailable",
                            Json::Value(Json::objectValue),
                            drogon::k500InternalServerError);
    }
    const auto message = state == AuthState::Missing ? "missing token" : "invalid token";
    return jsonResponse(401, message, Json::Value(Json::objectValue),
                        drogon::k401Unauthorized);
}

} // namespace mathai::utils
