#pragma once

#include <drogon/HttpFilter.h>

class UserAuthFilter : public drogon::HttpFilter<UserAuthFilter>
{
  public:
    void doFilter(const drogon::HttpRequestPtr &request,
                  drogon::FilterCallback &&filterCallback,
                  drogon::FilterChainCallback &&filterChainCallback) override;
};
