﻿﻿﻿#include "dao/ChapterDao.h"

#include <drogon/drogon.h>

#include <sstream>

#include "utils/MysqlSyncClient.h"

namespace mathai::dao
{
namespace
{

Json::Value chapterRowToJson(const mathai::utils::mysql::Row &row)
{
    Json::Value item;
    item["id"] = Json::Int64(row.getInt64("id"));
    item["courseId"] = Json::Int64(row.getInt64("course_id"));
    item["parentId"] = row.isNull("parent_id") ? Json::Value(Json::nullValue)
                                                : Json::Value(Json::Int64(row.getInt64("parent_id")));
    item["title"] = row.getString("title");
    item["description"] = row.getString("description");
    item["sortOrder"] = row.getInt("sort_order");
    item["status"] = row.getInt("status");
    return item;
}

Json::Value chapterRowToJsonWithCourseName(const mathai::utils::mysql::Row &row)
{
    Json::Value item = chapterRowToJson(row);
    item["courseName"] = row.getString("course_name");
    return item;
}

} // namespace

void ChapterDao::listActiveByCourse(long long courseId,
                                    int page,
                                    int pageSize,
                                    JsonCallback onSuccess,
                                    ErrorCallback onError) const
{
    try
    {
        const auto countResult = mathai::utils::mysql::execute(
            "SELECT COUNT(*) AS total FROM chapter WHERE course_id = " + std::to_string(courseId) + " AND status = 1");
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, course_id, parent_id, title, description, sort_order, status "
            "FROM chapter WHERE course_id = " + std::to_string(courseId) +
            " AND status = 1 ORDER BY sort_order ASC, id ASC LIMIT " +
            std::to_string(pageSize) + " OFFSET " + std::to_string(offset));

        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows)
        {
            items.append(chapterRowToJson(row));
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

void ChapterDao::listAll(const drogon::HttpRequestPtr &request,
                         int page, int pageSize,
                         JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        std::ostringstream whereClause;
        whereClause << " WHERE ch.status = 1";
        const auto courseId = request->getParameter("courseId");
        if (!courseId.empty())
        {
            whereClause << " AND ch.course_id = " << std::stoll(courseId);
        }

        const auto whereStr = whereClause.str();
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto countResult = mathai::utils::mysql::execute("SELECT COUNT(*) AS total FROM chapter ch" + whereStr);
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto result = mathai::utils::mysql::execute(
            "SELECT ch.id, ch.course_id, ch.parent_id, ch.title, ch.description, ch.sort_order, ch.status, "
            "c.name AS course_name FROM chapter ch LEFT JOIN course c ON ch.course_id = c.id" +
            whereStr + " ORDER BY ch.sort_order ASC, ch.id ASC LIMIT " + std::to_string(pageSize) +
            " OFFSET " + std::to_string(offset));

        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows)
        {
            items.append(chapterRowToJsonWithCourseName(row));
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

void ChapterDao::create(const Json::Value &chapter, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto parentId = chapter["parentId"].isNull() ? "NULL" : std::to_string(chapter["parentId"].asInt64());
        const auto result = mathai::utils::mysql::execute(
            "INSERT INTO chapter (course_id, parent_id, title, description, sort_order, status) VALUES (" +
            std::to_string(chapter["courseId"].asInt64()) + ", " + parentId + ", " +
            mathai::utils::mysql::quote(chapter["title"].asString()) + ", " +
            mathai::utils::mysql::quote(chapter.get("description", "").asString()) + ", " +
            std::to_string(chapter.get("sortOrder", 0).asInt()) + ", " +
            std::to_string(chapter.get("status", 1).asInt()) + ")");
        Json::Value data;
        data["id"] = Json::UInt64(result.insertId);
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void ChapterDao::update(long long id,
                        const Json::Value &chapter,
                        CountCallback onSuccess,
                        ErrorCallback onError) const
{
    try
    {
        const auto parentId = chapter["parentId"].isNull() ? "NULL" : std::to_string(chapter["parentId"].asInt64());
        const auto result = mathai::utils::mysql::execute(
            "UPDATE chapter SET course_id = " + std::to_string(chapter["courseId"].asInt64()) +
            ", parent_id = " + parentId +
            ", title = " + mathai::utils::mysql::quote(chapter["title"].asString()) +
            ", description = " + mathai::utils::mysql::quote(chapter.get("description", "").asString()) +
            ", sort_order = " + std::to_string(chapter.get("sortOrder", 0).asInt()) +
            ", status = " + std::to_string(chapter.get("status", 1).asInt()) +
            " WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void ChapterDao::disable(long long id, CountCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute("UPDATE chapter SET status = 0 WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

} // namespace mathai::dao
