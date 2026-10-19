#include "utils/AuthContext.h"

#include <drogon/drogon.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iterator>
#include <mutex>
#include <string>
#include <unordered_map>

#include "utils/JsonResponse.h"
#include "utils/MysqlSyncClient.h"

namespace mathai::utils
{
namespace
{

constexpr const char *claimsAttributeKey = "mathai.auth.claims";

struct VerifiedUser
{
    std::string role;
    int authVersion{0};
    std::chrono::steady_clock::time_point expiresAt;
};

long long configuredAuthCacheSeconds()
{
    const auto *value = std::getenv("AUTH_CACHE_TTL_SECONDS");
    if (value == nullptr || *value == '\0')
    {
        return 10;
    }
    try
    {
        return std::clamp<long long>(std::stoll(value), 0, 300);
    }
    catch (const std::exception &)
    {
        return 10;
    }
}

// Caches the "token is still valid for this user" database lookup for a few
// seconds. Without it every authenticated request pays an extra
// SELECT role, status, auth_version FROM user round trip on top of the work it
// actually needs to do. AUTH_CACHE_TTL_SECONDS=0 disables the cache and
// restores an exact database check on every request; the default of 10 keeps
// role/status/version changes (admin disable, password reset) invisible for at
// most 10 seconds.
class VerifiedUserCache
{
  public:
    VerifiedUserCache() : ttl_(std::chrono::seconds(configuredAuthCacheSeconds())) {}

    bool matches(long long userId, const std::string &role, int authVersion)
    {
        if (ttl_.count() == 0)
        {
            return false;
        }

        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = entries_.find(userId);
        if (found == entries_.end())
        {
            return false;
        }
        if (found->second.expiresAt <= now)
        {
            entries_.erase(found);
            return false;
        }
        return found->second.role == role && found->second.authVersion == authVersion;
    }

    void store(long long userId, const std::string &role, int authVersion)
    {
        if (ttl_.count() == 0)
        {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        if (entries_.size() >= maximumEntries_)
        {
            for (auto iterator = entries_.begin(); iterator != entries_.end();)
            {
                iterator = iterator->second.expiresAt <= now ? entries_.erase(iterator)
                                                             : std::next(iterator);
            }
        }
        entries_[userId] = {role, authVersion, now + ttl_};
    }

    void forget(long long userId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.erase(userId);
    }

  private:
    static constexpr std::size_t maximumEntries_ = 10000;
    std::mutex mutex_;
    std::unordered_map<long long, VerifiedUser> entries_;
    std::chrono::seconds ttl_;
};

VerifiedUserCache &verifiedUserCache()
{
    static VerifiedUserCache cache;
    return cache;
}

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

    if (verifiedUserCache().matches(claims->userId, claims->role, claims->authVersion))
    {
        attributes->insert(claimsAttributeKey, *claims);
        return {AuthState::Ok, claims};
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
            verifiedUserCache().forget(claims->userId);
            return {AuthState::Invalid, std::nullopt};
        }

        verifiedUserCache().store(claims->userId, claims->role, claims->authVersion);
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
