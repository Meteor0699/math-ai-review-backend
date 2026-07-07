#pragma once

#include <drogon/HttpFilter.h>

class AdminAuthFilter : public drogon::HttpFilter<AdminAuthFilter>
{
  public:
    void doFilter(const drogon::HttpRequestPtr &request,
                  drogon::FilterCallback &&filterCallback,
                  drogon::FilterChainCallback &&filterChainCallback) override;
};
