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
    explicit Error(const std::string &message) : std::runtime_error(message) {}
};

Result execute(const std::string &sql);
std::vector<Result> executeTransaction(const std::vector<std::string> &statements);
Json::Value configSummary();
std::string escape(const std::string &value);
std::string quote(const std::string &value);
std::string nullableString(const std::string &value);
std::string nullableInt64(const Json::Value &value);

} // namespace mathai::utils::mysql
