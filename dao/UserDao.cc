#include "dao/UserDao.h"

#include <drogon/drogon.h>
#include <sstream>
#include <vector>

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
            "SELECT id, username, password_hash, real_name, student_no, role, status, auth_version "
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
        user.authVersion = row.getInt("auth_version", 1);
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
        std::vector<std::string> assignments;
        bool invalidateSessions = false;
        if (user.isMember("realName"))
        {
            assignments.push_back("real_name = " + nullableText(user["realName"]));
        }
        if (user.isMember("studentNo"))
        {
            assignments.push_back("student_no = " + nullableText(user["studentNo"]));
        }
        if (user.isMember("role"))
        {
            assignments.push_back("role = " + mathai::utils::mysql::quote(user["role"].asString()));
            invalidateSessions = true;
        }
        if (user.isMember("status"))
        {
            assignments.push_back("status = " + std::to_string(user["status"].asInt()));
            invalidateSessions = true;
        }
        if (invalidateSessions)
        {
            assignments.push_back("auth_version = auth_version + 1");
        }

        std::ostringstream sql;
        sql << "UPDATE user SET ";
        for (std::size_t index = 0; index < assignments.size(); ++index)
        {
            if (index > 0) sql << ", ";
            sql << assignments[index];
        }
        sql << " WHERE id = " << id;
        const auto result = mathai::utils::mysql::execute(sql.str());
        if (result.affectedRows > 0)
        {
            onSuccess(result.affectedRows);
            return;
        }
        const auto existing = mathai::utils::mysql::execute(
            "SELECT 1 AS found FROM user WHERE id = " + std::to_string(id) + " LIMIT 1");
        onSuccess(existing.rows.empty() ? 0 : 1);
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
            ", auth_version = auth_version + 1" +
            " WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void UserDao::recordSuccessfulLogin(long long id,
                                    int authVersion,
                                    const std::string &upgradedPasswordHash,
                                    CountCallback onSuccess,
                                    ErrorCallback onError) const
{
    try
    {
        auto sql = std::string("UPDATE user SET last_login_at = CURRENT_TIMESTAMP");
        if (!upgradedPasswordHash.empty())
        {
            sql += ", password_hash = " + mathai::utils::mysql::quote(upgradedPasswordHash);
        }
        sql += " WHERE id = " + std::to_string(id) +
               " AND status = 1 AND auth_version = " + std::to_string(authVersion);
        const auto result = mathai::utils::mysql::execute(sql);
        if (result.affectedRows > 0)
        {
            onSuccess(result.affectedRows);
            return;
        }
        const auto existing = mathai::utils::mysql::execute(
            "SELECT 1 AS found FROM user WHERE id = " + std::to_string(id) +
            " AND status = 1 AND auth_version = " + std::to_string(authVersion) + " LIMIT 1");
        onSuccess(existing.rows.empty() ? 0 : 1);
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
            "UPDATE user SET status = 0, auth_version = auth_version + 1 WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

} // namespace mathai::dao
