#pragma once

#include <drogon/HttpFilter.h>

class RateLimitFilter : public drogon::HttpFilter<RateLimitFilter>
{
  public:
    void doFilter(const drogon::HttpRequestPtr &request,
                  drogon::FilterCallback &&filterCallback,
                  drogon::FilterChainCallback &&filterChainCallback) override;
};
