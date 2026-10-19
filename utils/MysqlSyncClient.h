#pragma once

#include <cstdint>
#include <json/json.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mathai::utils::mysql
{

struct Row
{
    std::unordered_map<std::string, std::string> values;
    std::unordered_set<std::string> nullFields;

    bool isNull(const std::string &field) const;
    std::string getString(const std::string &field, const std::string &fallback = "") const;
    long long getInt64(const std::string &field, long long fallback = 0) const;
    int getInt(const std::string &field, int fallback = 0) const;
};

struct Result
{
    std::vector<Row> rows;
    std::uint64_t affectedRows{0};
    std::uint64_t insertId{0};
};

class Error : public std::runtime_error
{
  public:
    explicit Error(const std::string &message, unsigned int code = 0)
        : std::runtime_error(message), code_(code)
    {
    }

    /// MySQL client error code (mysql_errno); 0 when the failure is local.
    unsigned int code() const noexcept { return code_; }

  private:
    unsigned int code_{0};
};

Result execute(const std::string &sql);
std::vector<Result> executeTransaction(const std::vector<std::string> &statements);
std::string escape(const std::string &value);
std::string quote(const std::string &value);

} // namespace mathai::utils::mysql
