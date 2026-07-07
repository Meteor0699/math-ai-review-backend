#pragma once

#include <drogon/HttpRequest.h>
#include <cstdint>
#include <functional>
#include <string>

#include <json/json.h>

namespace mathai::dao
{

class ChapterDao
{
  public:
    using JsonCallback = std::function<void(Json::Value)>;
    using CountCallback = std::function<void(std::uint64_t)>;
    using ErrorCallback = std::function<void(const std::string &)>;

    void listActiveByCourse(long long courseId, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const;
    void listAll(const drogon::HttpRequestPtr &request, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const;
    void create(const Json::Value &chapter, JsonCallback onSuccess, ErrorCallback onError) const;
    void update(long long id, const Json::Value &chapter, CountCallback onSuccess, ErrorCallback onError) const;
    void disable(long long id, CountCallback onSuccess, ErrorCallback onError) const;
};

} // namespace mathai::dao
