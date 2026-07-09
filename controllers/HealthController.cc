#include "controllers/HealthController.h"

#include <cstdlib>
#include <string>

#include "utils/JsonResponse.h"
#include "utils/MysqlSyncClient.h"

namespace
{

bool hasEnv(const char *name)
{
    const char *value = std::getenv(name);
    return value && value[0];
}

Json::Value envStatus()
{
    Json::Value env;
    env["DB_HOST"] = hasEnv("DB_HOST");
    env["DB_PORT"] = hasEnv("DB_PORT");
    env["DB_NAME"] = hasEnv("DB_NAME");
    env["DB_USER"] = hasEnv("DB_USER");
    env["DB_PASSWORD"] = hasEnv("DB_PASSWORD");
    env["MYSQLHOST"] = hasEnv("MYSQLHOST");
    env["MYSQLPORT"] = hasEnv("MYSQLPORT");
    env["MYSQLDATABASE"] = hasEnv("MYSQLDATABASE");
    env["MYSQLUSER"] = hasEnv("MYSQLUSER");
    env["MYSQLPASSWORD"] = hasEnv("MYSQLPASSWORD");
    env["DATABASE_URL"] = hasEnv("DATABASE_URL");
    env["MYSQL_URL"] = hasEnv("MYSQL_URL");
    env["DB_URL"] = hasEnv("DB_URL");
    env["MYSQL_PUBLIC_URL"] = hasEnv("MYSQL_PUBLIC_URL");
    return env;
}

} // namespace

void HealthController::db(const drogon::HttpRequestPtr &,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    Json::Value data;
    data["env"] = envStatus();
    data["config"] = mathai::utils::mysql::configSummary();

    try
    {
        mathai::utils::mysql::execute("SELECT 1 AS ok");
        data["connected"] = true;

        const auto userTable = mathai::utils::mysql::execute("SHOW TABLES LIKE 'user'");
        data["userTableExists"] = !userTable.rows.empty();

        if (!userTable.rows.empty())
        {
            const auto users = mathai::utils::mysql::execute(
                "SELECT "
                "SUM(username = 'admin') AS admin_count, "
                "SUM(username = 'student') AS student_count, "
                "COUNT(*) AS total_count "
                "FROM user");
            if (!users.rows.empty())
            {
                data["adminCount"] = users.rows[0].getInt("admin_count");
                data["studentCount"] = users.rows[0].getInt("student_count");
                data["totalUserCount"] = users.rows[0].getInt("total_count");
            }
        }

        callback(mathai::utils::jsonResponse(200, "success", data));
    }
    catch (const std::exception &e)
    {
        data["connected"] = false;
        data["error"] = std::string(e.what());
        callback(mathai::utils::jsonResponse(
            500,
            "database health check failed",
            data,
            drogon::k500InternalServerError));
    }
}
