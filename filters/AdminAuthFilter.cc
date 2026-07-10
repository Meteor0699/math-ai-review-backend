#include "filters/AdminAuthFilter.h"

#include "utils/AuthContext.h"
#include "utils/JsonResponse.h"

void AdminAuthFilter::doFilter(const drogon::HttpRequestPtr &request,
                               drogon::FilterCallback &&filterCallback,
                               drogon::FilterChainCallback &&filterChainCallback)
{
    const auto auth = mathai::utils::authenticateRequest(request);
    if (auth.state != mathai::utils::AuthState::Ok)
    {
        filterCallback(mathai::utils::authErrorResponse(auth.state));
        return;
    }

    if (auth.claims->role != "admin")
    {
        filterCallback(mathai::utils::jsonResponse(403, "admin permission required",
                                                   Json::Value(Json::objectValue),
                                                   drogon::k403Forbidden));
        return;
    }

    filterChainCallback();
}
