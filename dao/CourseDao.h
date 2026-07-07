#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <json/json.h>

namespace mathai::dao
{

class CourseDao
{
  public:
    using JsonCallback = std::function<void(Json::Value)>;
    using CountCallback = std::function<void(std::uint64_t)>;
    using ErrorCallback = std::function<void(const std::string &)>;

    void listActive(int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const;
    void findActiveById(long long id, JsonCallback onSuccess, ErrorCallback onError) const;
    void create(const Json::Value &course, JsonCallback onSuccess, ErrorCallback onError) const;
    void update(long long id, const Json::Value &course, CountCallback onSuccess, ErrorCallback onError) const;
    void disable(long long id, CountCallback onSuccess, ErrorCallback onError) const;
};

} // namespace mathai::dao
