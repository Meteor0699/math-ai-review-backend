#pragma once

#include <drogon/HttpController.h>

class HealthController : public drogon::HttpController<HealthController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::db, "/api/health/db", drogon::Get);
    METHOD_LIST_END

    void db(const drogon::HttpRequestPtr &request,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
