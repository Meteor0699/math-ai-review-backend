#pragma once

#include <drogon/HttpResponse.h>
#include <functional>
#include <json/json.h>

#include "dao/CourseDao.h"

namespace mathai::services
{

class CourseService
{
  public:
    using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

    void listActive(int page, int pageSize, ResponseCallback callback) const;
    void detail(long long id, ResponseCallback callback) const;
    void create(const Json::Value &requestBody, ResponseCallback callback) const;
    void update(long long id, const Json::Value &requestBody, ResponseCallback callback) const;
    void disable(long long id, ResponseCallback callback) const;

  private:
    mathai::dao::CourseDao courseDao_;
};

} // namespace mathai::services
