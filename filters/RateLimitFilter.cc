#include "filters/RateLimitFilter.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>

#include "utils/AuthContext.h"
#include "utils/JsonResponse.h"

namespace
{

struct Limit
{
    std::uint32_t maximum;
    std::chrono::seconds window;
    const char *scope;
};

struct Counter
{
    std::uint32_t count{0};
    std::chrono::steady_clock::time_point resetAt;
};

class RateLimiter
{
  public:
    bool allow(const std::string &key, const Limit &limit, std::uint64_t &retryAfter)
    {
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        auto &counter = counters_[key];
        if (counter.resetAt <= now)
        {
            counter.count = 0;
            counter.resetAt = now + limit.window;
        }
        if (counter.count >= limit.maximum)
        {
            retryAfter = static_cast<std::uint64_t>(
                std::max<std::int64_t>(1, std::chrono::duration_cast<std::chrono::seconds>(counter.resetAt - now).count()));
            return false;
        }
        ++counter.count;

        if (counters_.size() > 10000)
        {
            for (auto iterator = counters_.begin(); iterator != counters_.end();)
            {
                iterator = iterator->second.resetAt <= now ? counters_.erase(iterator) : std::next(iterator);
            }
        }
        return true;
    }

  private:
    std::mutex mutex_;
    std::unordered_map<std::string, Counter> counters_;
};

RateLimiter &limiter()
{
    static RateLimiter instance;
    return instance;
}

Limit limitForPath(const std::string &path)
{
    if (path.find("/auth/register") != std::string::npos)
        return {3, std::chrono::hours(1), "register"};
    if (path.find("/auth/login") != std::string::npos)
        return {10, std::chrono::minutes(1), "login"};
    if (path.find("/upload") != std::string::npos)
        return {5, std::chrono::minutes(10), "upload"};
    return {10, std::chrono::minutes(1), "ai"};
}

std::string trimSpaces(const std::string &value)
{
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string::npos)
    {
        return "";
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool trustsProxyHeaders()
{
    static const bool enabled = [] {
        const auto *value = std::getenv("TRUST_PROXY_HEADERS");
        if (value == nullptr)
        {
            return false;
        }
        const std::string text(value);
        return text == "1" || text == "true" || text == "TRUE" || text == "yes" || text == "YES";
    }();
    return enabled;
}

std::string forwardedClientAddress(const drogon::HttpRequestPtr &request)
{
    const auto forwardedFor = request->getHeader("X-Forwarded-For");
    if (!forwardedFor.empty())
    {
        // Exactly one trusted proxy is assumed, so the rightmost entry is the
        // address that proxy observed. Entries further left may have been
        // supplied by the client.
        const auto comma = forwardedFor.rfind(',');
        const auto candidate = trimSpaces(
            comma == std::string::npos ? forwardedFor : forwardedFor.substr(comma + 1));
        if (!candidate.empty())
        {
            return candidate;
        }
    }

    const auto realIp = trimSpaces(request->getHeader("X-Real-IP"));
    return realIp;
}

std::string clientAddress(const drogon::HttpRequestPtr &request)
{
    // Behind a reverse proxy (Render, nginx, Cloudflare) peerAddr() is the
    // proxy itself, so without forwarded headers every visitor shares a single
    // rate limit bucket and one noisy client locks out everybody. The headers
    // are only trusted when the deployment states that the backend is not
    // directly reachable, because they are client controlled otherwise.
    if (trustsProxyHeaders())
    {
        const auto forwarded = forwardedClientAddress(request);
        if (!forwarded.empty())
        {
            return forwarded;
        }
    }
    return request->peerAddr().toIp();
}

std::string requestIdentity(const drogon::HttpRequestPtr &request)
{
    const auto auth = mathai::utils::authenticateRequest(request);
    if (auth.state == mathai::utils::AuthState::Ok && auth.claims)
    {
        return "user:" + std::to_string(auth.claims->userId);
    }
    return "ip:" + clientAddress(request);
}

} // namespace

void RateLimitFilter::doFilter(const drogon::HttpRequestPtr &request,
                               drogon::FilterCallback &&filterCallback,
                               drogon::FilterChainCallback &&filterChainCallback)
{
    const auto limit = limitForPath(request->path());
    const auto key = std::string(limit.scope) + ":" + requestIdentity(request);
    std::uint64_t retryAfter = 0;
    if (!limiter().allow(key, limit, retryAfter))
    {
        auto response = mathai::utils::jsonResponse(429, "too many requests",
                                                    Json::Value(Json::objectValue),
                                                    drogon::k429TooManyRequests);
        response->addHeader("Retry-After", std::to_string(retryAfter));
        filterCallback(response);
        return;
    }
    filterChainCallback();
}
