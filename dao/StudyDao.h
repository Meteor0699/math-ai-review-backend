#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <json/json.h>

namespace mathai::dao
{

class StudyDao
{
  public:
    using JsonCallback = std::function<void(Json::Value)>;
    using CountCallback = std::function<void(std::uint64_t)>;
    using ErrorCallback = std::function<void(const std::string &)>;

    void findState(long long userId, long long questionId, JsonCallback onSuccess, ErrorCallback onError) const;
    void upsertAttempt(long long userId,
                       long long questionId,
                       const std::string &status,
                       const std::string &note,
                       JsonCallback onSuccess,
                       ErrorCallback onError) const;
    void listAttempts(long long userId, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const;
    void addWrong(long long userId,
                  long long questionId,
                  const std::string &note,
                  JsonCallback onSuccess,
                  ErrorCallback onError) const;
    void removeWrong(long long userId, long long questionId, CountCallback onSuccess, ErrorCallback onError) const;
    void listWrong(long long userId, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const;
};

} // namespace mathai::dao
