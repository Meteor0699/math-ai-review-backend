#include "dao/UserDao.h"

#include <drogon/drogon.h>
#include <sstream>

#include "utils/MysqlSyncClient.h"

namespace mathai::dao
{
namespace
{

Json::Value userRowToJson(const mathai::utils::mysql::Row &row)
{
    Json::Value item;
    item["id"] = Json::Int64(row.getInt64("id"));
    item["username"] = row.getString("username");
    item["realName"] = row.getString("real_name");
    item["studentNo"] = row.getString("student_no");
    item["role"] = row.getString("role");
    item["status"] = row.getInt("status");
    item["lastLoginAt"] = row.getString("last_login_at");
    item["createdAt"] = row.getString("created_at");
    item["updatedAt"] = row.getString("updated_at");
    return item;
}

std::string nullableText(const Json::Value &value)
{
    if (value.isNull() || !value.isString() || value.asString().empty())
    {
        return "NULL";
    }
    return mathai::utils::mysql::quote(value.asString());
}

} // namespace

void UserDao::findByUsername(const std::string &username,
                             UserCallback onSuccess,
                             ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, username, password_hash, real_name, student_no, role, status "
            "FROM user WHERE username = " + mathai::utils::mysql::quote(username) + " LIMIT 1");
        if (result.rows.empty())
        {
            onSuccess(std::nullopt);
            return;
        }

        const auto &row = result.rows[0];
        mathai::models::User user;
        user.id = row.getInt64("id");
        user.username = row.getString("username");
        user.passwordHash = row.getString("password_hash");
        user.realName = row.getString("real_name");
        user.studentNo = row.getString("student_no");
        user.role = row.getString("role");
        user.status = row.getInt("status");
        onSuccess(user);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void UserDao::list(const std::string &keyword,
                   const std::string &role,
                   int page,
                   int pageSize,
                   JsonCallback onSuccess,
                   ErrorCallback onError) const
{
    try
    {
        std::ostringstream where;
        where << " WHERE 1 = 1";
        if (!keyword.empty())
        {
            const auto like = "'%" + mathai::utils::mysql::escape(keyword) + "%'";
            where << " AND (username LIKE " << like
                  << " OR real_name LIKE " << like
                  << " OR student_no LIKE " << like << ")";
        }
        if (role == "student" || role == "admin")
        {
            where << " AND role = " << mathai::utils::mysql::quote(role);
        }

        const auto offset = (page - 1) * pageSize;
        const auto countResult = mathai::utils::mysql::execute("SELECT COUNT(*) AS total FROM user" + where.str());
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, username, real_name, student_no, role, status, last_login_at, created_at, updated_at "
            "FROM user" + where.str() +
            " ORDER BY id DESC LIMIT " + std::to_string(pageSize) +
            " OFFSET " + std::to_string(offset));

        Json::Value data;
        data["items"] = Json::Value(Json::arrayValue);
        data["total"] = Json::UInt64(countResult.rows.empty() ? 0 : countResult.rows[0].getInt64("total"));
        for (const auto &row : result.rows)
        {
            data["items"].append(userRowToJson(row));
        }
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void UserDao::create(const Json::Value &user,
                     JsonCallback onSuccess,
                     ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "INSERT INTO user (username, password_hash, real_name, student_no, role, status) VALUES (" +
            mathai::utils::mysql::quote(user["username"].asString()) + ", " +
            mathai::utils::mysql::quote(user["passwordHash"].asString()) + ", " +
            nullableText(user.get("realName", Json::Value(Json::nullValue))) + ", " +
            nullableText(user.get("studentNo", Json::Value(Json::nullValue))) + ", " +
            mathai::utils::mysql::quote(user.get("role", "student").asString()) + ", " +
            std::to_string(user.get("status", 1).asInt()) + ")");
        Json::Value data;
        data["id"] = Json::UInt64(result.insertId);
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void UserDao::update(long long id,
                     const Json::Value &user,
                     CountCallback onSuccess,
                     ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "UPDATE user SET real_name = " + nullableText(user.get("realName", Json::Value(Json::nullValue))) +
            ", student_no = " + nullableText(user.get("studentNo", Json::Value(Json::nullValue))) +
            ", role = " + mathai::utils::mysql::quote(user.get("role", "student").asString()) +
            ", status = " + std::to_string(user.get("status", 1).asInt()) +
            " WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void UserDao::updatePassword(long long id,
                             const std::string &passwordHash,
                             CountCallback onSuccess,
                             ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "UPDATE user SET password_hash = " + mathai::utils::mysql::quote(passwordHash) +
            " WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void UserDao::disable(long long id,
                      CountCallback onSuccess,
                      ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "UPDATE user SET status = 0 WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

} // namespace mathai::dao
