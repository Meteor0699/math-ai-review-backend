#include "filters/UserAuthFilter.h"

#include "utils/AuthContext.h"

void UserAuthFilter::doFilter(const drogon::HttpRequestPtr &request,
                              drogon::FilterCallback &&filterCallback,
                              drogon::FilterChainCallback &&filterChainCallback)
{
    const auto auth = mathai::utils::authenticateRequest(request);
    if (auth.state != mathai::utils::AuthState::Ok)
    {
        filterCallback(mathai::utils::authErrorResponse(auth.state));
        return;
    }

    filterChainCallback();
}
