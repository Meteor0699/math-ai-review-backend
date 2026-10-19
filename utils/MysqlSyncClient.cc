#include "utils/MysqlSyncClient.h"

#include <drogon/drogon.h>
#include <errmsg.h>
#include <mysql.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>

namespace
{

std::string getenvString(const char *name, const std::string &fallback = "")
{
    const char *value = std::getenv(name);
    if (!value)
    {
        return fallback;
    }

    std::string text(value);
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return "";
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

unsigned int getenvUInt(const char *name, unsigned int fallback)
{
    const auto value = getenvString(name);
    if (value.empty())
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

std::string urlDecode(const std::string &value)
{
    std::string output;
    output.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '%' && i + 2 < value.size())
        {
            const auto hex = value.substr(i + 1, 2);
            try
            {
                output.push_back(static_cast<char>(std::stoul(hex, nullptr, 16)));
                i += 2;
                continue;
            }
            catch (const std::exception &)
            {
            }
        }
        output.push_back(value[i]);
    }
    return output;
}

std::string toUpper(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
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
    std::string password;
};

void applyUrlConfig(DbConfig &config, const std::string &url);

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

    const auto url = getenvString("DATABASE_URL",
                     getenvString("MYSQL_URL",
                     getenvString("DB_URL",
                     getenvString("MYSQL_PUBLIC_URL", ""))));
    if (!url.empty())
    {
        applyUrlConfig(config, url);
    }

    if (config.host.find("://") != std::string::npos)
    {
        applyUrlConfig(config, config.host);
    }
}

void applyUrlConfig(DbConfig &config, const std::string &url)
{
    const auto schemePos = url.find("://");
    if (schemePos == std::string::npos)
    {
        return;
    }

    const auto authorityStart = schemePos + 3;
    const auto pathStart = url.find('/', authorityStart);
    const auto authority = pathStart == std::string::npos
        ? url.substr(authorityStart)
        : url.substr(authorityStart, pathStart - authorityStart);
    auto path = pathStart == std::string::npos ? "" : url.substr(pathStart + 1);
    const auto queryPos = path.find('?');
    if (queryPos != std::string::npos)
    {
        path = path.substr(0, queryPos);
    }
    if (!path.empty())
    {
        config.dbname = urlDecode(path);
    }

    const auto atPos = authority.rfind('@');
    const auto userInfo = atPos == std::string::npos ? "" : authority.substr(0, atPos);
    const auto hostPort = atPos == std::string::npos ? authority : authority.substr(atPos + 1);
    if (!userInfo.empty())
    {
        const auto colonPos = userInfo.find(':');
        config.user = urlDecode(colonPos == std::string::npos ? userInfo : userInfo.substr(0, colonPos));
        if (colonPos != std::string::npos)
        {
            config.password = urlDecode(userInfo.substr(colonPos + 1));
        }
    }

    if (!hostPort.empty())
    {
        if (hostPort.front() == '[')
        {
            const auto endBracket = hostPort.find(']');
            if (endBracket != std::string::npos)
            {
                config.host = hostPort.substr(1, endBracket - 1);
                if (endBracket + 2 <= hostPort.size() && hostPort[endBracket + 1] == ':')
                {
                    try
                    {
                        config.port = static_cast<unsigned int>(std::stoul(hostPort.substr(endBracket + 2)));
                    }
                    catch (const std::exception &)
                    {
                    }
                }
            }
            return;
        }

        const auto colonPos = hostPort.rfind(':');
        if (colonPos == std::string::npos)
        {
            config.host = hostPort;
        }
        else
        {
            config.host = hostPort.substr(0, colonPos);
            try
            {
                config.port = static_cast<unsigned int>(std::stoul(hostPort.substr(colonPos + 1)));
            }
            catch (const std::exception &)
            {
            }
        }
    }
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

[[noreturn]] void throwMysqlError(MYSQL *connection, const char *context)
{
    const auto code = mysql_errno(connection);
    const std::string message = mysql_error(connection);
    throw Error(std::string(context) + ": " + message, code);
}

MysqlPtr createConnection()
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

    const std::string sslMode = toUpper(getenvString("DB_SSL_MODE", "DISABLED"));
    const bool verifyServerCertificate = sslMode == "VERIFY_CA" || sslMode == "VERIFY_IDENTITY";
    if (sslMode == "REQUIRED" || sslMode == "PREFERRED" || verifyServerCertificate)
    {
        const auto sslCa = getenvString("DB_SSL_CA");
        mysql_ssl_set(conn.get(),
                      nullptr,
                      nullptr,
                      sslCa.empty() ? nullptr : sslCa.c_str(),
                      nullptr,
                      nullptr);
        my_bool enforceSsl = 1;
        my_bool enforceVerify = verifyServerCertificate ? 1 : 0;
        mysql_optionsv(conn.get(), MYSQL_OPT_SSL_ENFORCE, &enforceSsl);
        mysql_optionsv(conn.get(), MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &enforceVerify);

        if (!verifyServerCertificate)
        {
            static std::once_flag warned;
            std::call_once(warned, [] {
                LOG_WARN << "DB_SSL_MODE=" << getenvString("DB_SSL_MODE", "DISABLED")
                         << " encrypts the connection but accepts any server certificate; use "
                            "DB_SSL_MODE=VERIFY_CA with DB_SSL_CA, or VERIFY_IDENTITY, to prevent MITM";
            });
        }
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
        throwMysqlError(conn.get(), "mysql_real_connect failed");
    }

    if (mysql_set_character_set(conn.get(), "utf8mb4") != 0)
    {
        throwMysqlError(conn.get(), "failed to select utf8mb4");
    }

    static const std::string normalizeSqlMode =
        "SET SESSION sql_mode = TRIM(BOTH ',' FROM REPLACE(CONCAT(',', @@SESSION.sql_mode, ','), "
        "',NO_BACKSLASH_ESCAPES,', ','))";
    if (mysql_real_query(conn.get(), normalizeSqlMode.c_str(),
                         static_cast<unsigned long>(normalizeSqlMode.size())) != 0)
    {
        throwMysqlError(conn.get(), "failed to normalize sql_mode");
    }
    return conn;
}

class ConnectionPool;

class ConnectionLease
{
  public:
    ConnectionLease(MYSQL *connection, ConnectionPool *pool)
        : connection_(connection), pool_(pool)
    {
    }
    ConnectionLease(const ConnectionLease &) = delete;
    ConnectionLease &operator=(const ConnectionLease &) = delete;
    ConnectionLease(ConnectionLease &&other) noexcept
        : connection_(other.connection_), pool_(other.pool_), healthy_(other.healthy_)
    {
        other.connection_ = nullptr;
    }
    ~ConnectionLease();

    MYSQL *get() const { return connection_; }
    void invalidate() { healthy_ = false; }

  private:
    MYSQL *connection_{};
    ConnectionPool *pool_{};
    bool healthy_{true};
};

struct IdleConnection
{
    MYSQL *handle;
    std::chrono::steady_clock::time_point idleSince;
};

// A pooled connection that has been idle for longer than this window is
// re-validated with mysql_ping before it is handed out. Pinging on every
// acquire costs one extra round trip per query, which nearly doubles the
// latency of each database-backed request. Set DB_POOL_IDLE_PING_SECONDS=0 to
// restore the old ping-on-every-acquire behaviour.
std::chrono::steady_clock::duration idlePingInterval()
{
    static const auto interval = std::chrono::seconds(
        std::clamp<std::size_t>(getenvUInt("DB_POOL_IDLE_PING_SECONDS", 60), 0, 3600));
    return interval;
}

class ConnectionPool
{
  public:
    ConnectionPool()
        : maximumSize_(std::clamp<std::size_t>(getenvUInt("DB_POOL_SIZE", 8), 1, 64)),
          acquireTimeout_(std::chrono::milliseconds(
              std::clamp<std::size_t>(getenvUInt("DB_POOL_ACQUIRE_TIMEOUT_MS", 3000), 100, 30000)))
    {
    }

    ~ConnectionPool()
    {
        for (const auto &entry : idle_)
        {
            mysql_close(entry.handle);
        }
    }

    ConnectionLease acquire()
    {
        while (true)
        {
            MYSQL *connection = nullptr;
            bool createNew = false;
            bool needsPing = false;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (!available_.wait_for(lock, acquireTimeout_,
                                         [this] { return !idle_.empty() || total_ < maximumSize_; }))
                {
                    throw mathai::utils::mysql::Error("database connection pool exhausted");
                }
                if (!idle_.empty())
                {
                    connection = idle_.back().handle;
                    const auto idleSince = idle_.back().idleSince;
                    idle_.pop_back();
                    const auto interval = idlePingInterval();
                    needsPing = interval.count() == 0 ||
                                std::chrono::steady_clock::now() - idleSince >= interval;
                }
                else
                {
                    ++total_;
                    createNew = true;
                }
            }

            if (createNew)
            {
                try
                {
                    connection = createConnection().release();
                }
                catch (...)
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    --total_;
                    available_.notify_one();
                    throw;
                }
            }
            else if (needsPing && mysql_ping(connection) != 0)
            {
                mysql_close(connection);
                std::lock_guard<std::mutex> lock(mutex_);
                --total_;
                available_.notify_one();
                continue;
            }
            return ConnectionLease(connection, this);
        }
    }

    void release(MYSQL *connection, bool healthy)
    {
        if (!connection)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (healthy)
        {
            idle_.push_back({connection, std::chrono::steady_clock::now()});
        }
        else
        {
            mysql_close(connection);
            --total_;
        }
        available_.notify_one();
    }

  private:
    std::mutex mutex_;
    std::condition_variable available_;
    std::vector<IdleConnection> idle_;
    std::size_t total_{0};
    std::size_t maximumSize_;
    std::chrono::milliseconds acquireTimeout_;
};

ConnectionLease::~ConnectionLease()
{
    if (pool_)
    {
        pool_->release(connection_, healthy_);
    }
}

ConnectionPool &connectionPool()
{
    static ConnectionPool pool;
    return pool;
}

// True when the failure is a broken/dead transport rather than a rejected
// statement. Only these may discard the pooled connection.
bool isConnectionLevelError(unsigned int code)
{
    switch (code)
    {
    case CR_SERVER_GONE_ERROR:
    case CR_SERVER_LOST:
    case CR_SERVER_HANDSHAKE_ERR:
    case CR_CONN_HOST_ERROR:
    case CR_CONNECTION_ERROR:
        return true;
    default:
        return false;
    }
}

Result executeOnConnection(MYSQL *connection, const std::string &sql)
{
    if (mysql_real_query(connection, sql.c_str(), static_cast<unsigned long>(sql.size())) != 0)
    {
        throwMysqlError(connection, "mysql_real_query failed");
    }

    Result output;
    output.affectedRows = mysql_affected_rows(connection);
    output.insertId = mysql_insert_id(connection);

    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res(mysql_store_result(connection),
                                                                 mysql_free_result);
    if (!res)
    {
        if (mysql_errno(connection) != 0)
        {
            throwMysqlError(connection, "mysql_store_result failed");
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

    long long parsed{};
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
    {
        return fallback;
    }
    return parsed;
}

int Row::getInt(const std::string &field, int fallback) const
{
    const auto value = getString(field);
    if (value.empty())
    {
        return fallback;
    }

    int parsed{};
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
    {
        return fallback;
    }
    return parsed;
}

Result execute(const std::string &sql)
{
    for (int attempt = 0;; ++attempt)
    {
        auto conn = connectionPool().acquire();
        try
        {
            return executeOnConnection(conn.get(), sql);
        }
        catch (const Error &error)
        {
            if (!isConnectionLevelError(error.code()))
            {
                // Statement level failure (syntax, constraint, bad data): the
                // connection itself is fine, so keep it in the pool.
                throw;
            }

            conn.invalidate();

            // CR_SERVER_GONE_ERROR means the statement was never delivered to
            // the server, so replaying it once on a fresh connection is safe.
            // Other transport failures leave the outcome unknown, so they are
            // not retried to avoid applying a write twice.
            if (error.code() == CR_SERVER_GONE_ERROR && attempt == 0)
            {
                LOG_WARN << "MySQL server closed an idle pooled connection; retrying the statement once";
                continue;
            }
            throw;
        }
    }
}

std::vector<Result> executeTransaction(const std::vector<std::string> &statements)
{
    auto conn = connectionPool().acquire();
    if (mysql_autocommit(conn.get(), 0) != 0)
    {
        throw Error(mysql_error(conn.get()));
    }

    std::vector<Result> results;
    results.reserve(statements.size());
    try
    {
        for (const auto &statement : statements)
        {
            results.push_back(executeOnConnection(conn.get(), statement));
        }
        if (mysql_commit(conn.get()) != 0)
        {
            throw Error(mysql_error(conn.get()));
        }
        if (mysql_autocommit(conn.get(), 1) != 0)
        {
            conn.invalidate();
            throw Error(mysql_error(conn.get()));
        }
    }
    catch (...)
    {
        mysql_rollback(conn.get());
        if (mysql_autocommit(conn.get(), 1) != 0)
        {
            conn.invalidate();
        }
        throw;
    }
    return results;
}

std::string escape(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size() * 2);
    for (const auto ch : value)
    {
        switch (ch)
        {
        case '\0': escaped += "\\0"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\\': escaped += "\\\\"; break;
        case '\'': escaped += "\\'"; break;
        case '"': escaped += "\\\""; break;
        case '\x1a': escaped += "\\Z"; break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

std::string quote(const std::string &value)
{
    return "'" + escape(value) + "'";
}

} // namespace mathai::utils::mysql
