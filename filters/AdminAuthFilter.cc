#include "filters/AdminAuthFilter.h"

#include "utils/JwtUtil.h"
#include "utils/JsonResponse.h"

void AdminAuthFilter::doFilter(const drogon::HttpRequestPtr &request,
                               drogon::FilterCallback &&filterCallback,
                               drogon::FilterChainCallback &&filterChainCallback)
{
    const auto authHeader = request->getHeader("Authorization");
    const std::string prefix = "Bearer ";
    if (authHeader.rfind(prefix, 0) != 0)
    {
        filterCallback(mathai::utils::jsonResponse(401, "missing token",
                                                   Json::Value(Json::objectValue),
                                                   drogon::k401Unauthorized));
        return;
    }

    const auto claims = mathai::utils::verifyJwt(authHeader.substr(prefix.size()));
    if (!claims)
    {
        filterCallback(mathai::utils::jsonResponse(401, "invalid token",
                                                   Json::Value(Json::objectValue),
                                                   drogon::k401Unauthorized));
        return;
    }

    if (claims->role != "admin")
    {
        filterCallback(mathai::utils::jsonResponse(403, "admin permission required",
                                                   Json::Value(Json::objectValue),
                                                   drogon::k403Forbidden));
        return;
    }

    filterChainCallback();
}
