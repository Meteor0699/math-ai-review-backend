#include "filters/UserAuthFilter.h"

#include "utils/JwtUtil.h"
#include "utils/JsonResponse.h"

void UserAuthFilter::doFilter(const drogon::HttpRequestPtr &request,
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

    filterChainCallback();
}
