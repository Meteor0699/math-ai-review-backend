#include "controllers/HealthController.h"

#include "utils/JsonResponse.h"
#include "utils/MysqlSyncClient.h"

void HealthController::db(const drogon::HttpRequestPtr &,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    Json::Value data;

    try
    {
        mathai::utils::mysql::execute("SELECT 1 AS ok");
        data["connected"] = true;
        callback(mathai::utils::jsonResponse(200, "success", data));
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Database health check failed: " << exception.what();
        data["connected"] = false;
        callback(mathai::utils::jsonResponse(
            503,
            "service unavailable",
            data,
            drogon::k503ServiceUnavailable));
    }
}
