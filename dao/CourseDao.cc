#include "dao/CourseDao.h"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "utils/MysqlSyncClient.h"

namespace mathai::dao
{
namespace
{

Json::Value courseRowToJson(const mathai::utils::mysql::Row &row)
{
    Json::Value item;
    item["id"] = Json::Int64(row.getInt64("id"));
    item["name"] = row.getString("name");
    item["code"] = row.getString("code");
    item["description"] = row.getString("description");
    item["courseName"] = item["name"];
    item["courseCode"] = item["code"];
    item["courseDesc"] = item["description"];
    item["coverUrl"] = row.getString("cover_url");
    item["sortOrder"] = row.getInt("sort_order");
    item["status"] = row.getInt("status");
    return item;
}

void handleDbError(CourseDao::ErrorCallback onError,
                   const drogon::orm::DrogonDbException &exception)
{
    onError(exception.base().what());
}

} // namespace

void CourseDao::listActive(int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto countResult = mathai::utils::mysql::execute(
            "SELECT COUNT(*) AS total FROM course WHERE status = 1");
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, name, code, description, cover_url, sort_order, status "
            "FROM course WHERE status = 1 ORDER BY sort_order ASC, id ASC LIMIT " +
            std::to_string(pageSize) + " OFFSET " + std::to_string(offset));

        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows)
        {
            items.append(courseRowToJson(row));
        }
        Json::Value data;
        data["items"] = items;
        data["total"] = total;
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void CourseDao::findActiveById(long long id, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, name, code, description, cover_url, sort_order, status "
            "FROM course WHERE id = " + std::to_string(id) + " AND status = 1 LIMIT 1");
        if (result.rows.empty())
        {
            onSuccess(Json::Value(Json::nullValue));
            return;
        }
        onSuccess(courseRowToJson(result.rows[0]));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void CourseDao::create(const Json::Value &course, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
        "INSERT INTO course (name, code, description, cover_url, sort_order, status) "
        "VALUES (" + mathai::utils::mysql::quote(course["name"].asString()) + ", " +
        mathai::utils::mysql::quote(course["code"].asString()) + ", " +
        mathai::utils::mysql::quote(course.get("description", "").asString()) + ", " +
        mathai::utils::mysql::quote(course.get("coverUrl", "").asString()) + ", " +
        std::to_string(course.get("sortOrder", 0).asInt()) + ", " +
        std::to_string(course.get("status", 1).asInt()) + ")");
        Json::Value data;
        data["id"] = Json::UInt64(result.insertId);
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void CourseDao::update(long long id,
                       const Json::Value &course,
                       CountCallback onSuccess,
                       ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "UPDATE course SET name = " + mathai::utils::mysql::quote(course["name"].asString()) +
            ", code = " + mathai::utils::mysql::quote(course["code"].asString()) +
            ", description = " + mathai::utils::mysql::quote(course.get("description", "").asString()) +
            ", cover_url = " + mathai::utils::mysql::quote(course.get("coverUrl", "").asString()) +
            ", sort_order = " + std::to_string(course.get("sortOrder", 0).asInt()) +
            ", status = " + std::to_string(course.get("status", 1).asInt()) +
            " WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void CourseDao::disable(long long id, CountCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "UPDATE course SET status = 0 WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

} // namespace mathai::dao
