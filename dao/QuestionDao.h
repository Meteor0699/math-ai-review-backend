#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <drogon/HttpRequest.h>
#include <json/json.h>

namespace mathai::dao
{

class QuestionDao
{
  public:
    using JsonCallback = std::function<void(Json::Value)>;
    using CountCallback = std::function<void(std::uint64_t)>;
    using ErrorCallback = std::function<void(const std::string &)>;

    void listActive(const drogon::HttpRequestPtr &request, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const;
    void listAdmin(const drogon::HttpRequestPtr &request, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const;
    void findActiveById(long long id, JsonCallback onSuccess, ErrorCallback onError) const;
    void listOptions(long long questionId, JsonCallback onSuccess, ErrorCallback onError) const;
    void create(const Json::Value &question, JsonCallback onSuccess, ErrorCallback onError) const;
    void update(long long id, const Json::Value &question, CountCallback onSuccess, ErrorCallback onError) const;
    void disable(long long id, CountCallback onSuccess, ErrorCallback onError) const;
    void addOptions(long long questionId, const Json::Value &options, JsonCallback onSuccess, ErrorCallback onError) const;
    void replaceOptions(long long questionId, const Json::Value &options, JsonCallback onSuccess, ErrorCallback onError) const;
    void deleteOption(long long optionId, CountCallback onSuccess, ErrorCallback onError) const;
};

} // namespace mathai::dao
