#pragma once

#include <drogon/HttpController.h>

class AdminStatsController : public drogon::HttpController<AdminStatsController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminStatsController::summary, "/api/admin/stats", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(AdminStatsController::summary, "/api/v1/admin/stats", drogon::Get, "AdminAuthFilter");
    METHOD_LIST_END

    void summary(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
