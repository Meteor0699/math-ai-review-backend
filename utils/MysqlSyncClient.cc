#include "utils/MysqlSyncClient.h"

#include <drogon/drogon.h>
#include <mysql.h>

#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>

namespace
{

std::string getenvString(const char *name, const std::string &fallback = "")
{
    const char *value = std::getenv(name);
    return value ? std::string(value) : fallback;
}

unsigned int getenvUInt(const char *name, unsigned int fallback)
{
    const char *value = std::getenv(name);
    if (!value || !value[0])
    {
        return fallback;
    }
    try
    {
        return static_cast<unsigned int>(std::stoul(value));
    }
    catch (const std::exception &)
    {
        return fallback;
    }
}

}

namespace mathai::utils::mysql
{
namespace
{

struct DbConfig
{
    std::string host{"127.0.0.1"};
    unsigned int port{3306};
    std::string dbname{"math_ai_review"};
    std::string user{"root"};
    std::string password{"123456"};
};

void applyEnvOverrides(DbConfig &config)
{
    // Explicit deployment variables.
    config.host = getenvString("DB_HOST", config.host);
    config.port = getenvUInt("DB_PORT", config.port);
    config.dbname = getenvString("DB_NAME", config.dbname);
    config.user = getenvString("DB_USER", config.user);
    config.password = getenvString("DB_PASSWORD", config.password);

    // Common MySQL hosting variables.
    config.host = getenvString("MYSQLHOST", config.host);
    config.port = getenvUInt("MYSQLPORT", config.port);
    config.dbname = getenvString("MYSQLDATABASE", config.dbname);
    config.user = getenvString("MYSQLUSER", config.user);
    config.password = getenvString("MYSQLPASSWORD", config.password);
}

DbConfig loadConfig()
{
    static std::once_flag once;
    static DbConfig config;
    static std::string error;

    std::call_once(once, [] {
        std::ifstream input("config.json");
        if (input)
        {
            Json::Value root;
            Json::CharReaderBuilder builder;
            if (!Json::parseFromStream(builder, input, &root, &error))
            {
                return;
            }

            const auto clients = root["db_clients"];
            if (clients.isArray() && !clients.empty())
            {
                const auto &db = clients[0U];
                config.host = db.get("host", config.host).asString();
                config.port = db.get("port", static_cast<int>(config.port)).asUInt();
                config.dbname = db.get("dbname", config.dbname).asString();
                config.user = db.get("user", config.user).asString();
                config.password = db.get("passwd", config.password).asString();
            }
        }

        applyEnvOverrides(config);
    });

    if (!error.empty())
    {
        throw Error("failed to parse config.json: " + error);
    }
    return config;
}

using MysqlPtr = std::unique_ptr<MYSQL, decltype(&mysql_close)>;

MysqlPtr connect()
{
    const auto config = loadConfig();
    MYSQL *raw = mysql_init(nullptr);
    if (!raw)
    {
        throw Error("mysql_init failed");
    }

    MysqlPtr conn(raw, mysql_close);
    unsigned int timeout = 5;
    mysql_options(conn.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(conn.get(), MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(conn.get(), MYSQL_OPT_WRITE_TIMEOUT, &timeout);

    const std::string sslMode = getenvString("DB_SSL_MODE", "DISABLED");
    if (sslMode == "REQUIRED" || sslMode == "PREFERRED")
    {
        mysql_ssl_set(conn.get(), nullptr, nullptr, getenvString("DB_SSL_CA", "").c_str(), nullptr, nullptr);
        my_bool enforceSsl = 1;
        mysql_optionsv(conn.get(), MYSQL_OPT_SSL_ENFORCE, &enforceSsl);
    }
    else
    {
        my_bool disableSsl = 0;
        mysql_optionsv(conn.get(), MYSQL_OPT_SSL_ENFORCE, &disableSsl);
        mysql_optionsv(conn.get(), MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &disableSsl);
    }

    if (!mysql_real_connect(conn.get(),
                            config.host.c_str(),
                            config.user.c_str(),
                            config.password.c_str(),
                            config.dbname.c_str(),
                            config.port,
                            nullptr,
                            0))
    {
        throw Error(mysql_error(conn.get()));
    }

    mysql_set_character_set(conn.get(), "utf8mb4");
    return conn;
}

} // namespace

bool Row::isNull(const std::string &field) const
{
    return nullFields.find(field) != nullFields.end();
}

std::string Row::getString(const std::string &field, const std::string &fallback) const
{
    const auto it = values.find(field);
    if (it == values.end() || isNull(field))
    {
        return fallback;
    }
    return it->second;
}

long long Row::getInt64(const std::string &field, long long fallback) const
{
    const auto value = getString(field);
    if (value.empty())
    {
        return fallback;
    }
    return std::stoll(value);
}

int Row::getInt(const std::string &field, int fallback) const
{
    const auto value = getString(field);
    if (value.empty())
    {
        return fallback;
    }
    return std::stoi(value);
}

Result execute(const std::string &sql)
{
    auto conn = connect();
    if (mysql_real_query(conn.get(), sql.c_str(), static_cast<unsigned long>(sql.size())) != 0)
    {
        throw Error(mysql_error(conn.get()));
    }

    Result output;
    output.affectedRows = mysql_affected_rows(conn.get());
    output.insertId = mysql_insert_id(conn.get());

    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res(mysql_store_result(conn.get()),
                                                                 mysql_free_result);
    if (!res)
    {
        if (mysql_errno(conn.get()) != 0)
        {
            throw Error(mysql_error(conn.get()));
        }
        return output;
    }

    const auto fieldCount = mysql_num_fields(res.get());
    MYSQL_FIELD *fields = mysql_fetch_fields(res.get());
    MYSQL_ROW mysqlRow;
    while ((mysqlRow = mysql_fetch_row(res.get())) != nullptr)
    {
        unsigned long *lengths = mysql_fetch_lengths(res.get());
        Row row;
        for (unsigned int i = 0; i < fieldCount; ++i)
        {
            const std::string fieldName = fields[i].name;
            if (mysqlRow[i] == nullptr)
            {
                row.nullFields.insert(fieldName);
                row.values[fieldName] = "";
            }
            else
            {
                row.values[fieldName] = std::string(mysqlRow[i], lengths[i]);
            }
        }
        output.rows.push_back(std::move(row));
    }

    return output;
}

std::string escape(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size() * 2);
    for (const auto ch : value)
    {
        if (ch == '\'' || ch == '\\')
        {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

std::string quote(const std::string &value)
{
    return "'" + escape(value) + "'";
}

std::string nullableString(const std::string &value)
{
    return value.empty() ? "NULL" : quote(value);
}

std::string nullableInt64(const Json::Value &value)
{
    if (value.isNull())
    {
        return "NULL";
    }
    return std::to_string(value.asInt64());
}

} // namespace mathai::utils::mysql
