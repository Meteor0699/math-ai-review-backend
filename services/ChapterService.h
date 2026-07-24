#pragma once

#include <drogon/HttpResponse.h>
#include <functional>
#include <json/json.h>

#include "dao/ChapterDao.h"

namespace mathai::services
{

class ChapterService
{
  public:
    using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

    void listActiveByCourse(long long courseId, int page, int pageSize, ResponseCallback callback) const;
    void detail(long long id, ResponseCallback callback) const;
    void listAll(const drogon::HttpRequestPtr &request, int page, int pageSize, ResponseCallback callback) const;
    void create(const Json::Value &requestBody, ResponseCallback callback) const;
    void update(long long id, const Json::Value &requestBody, ResponseCallback callback) const;
    void disable(long long id, ResponseCallback callback) const;

  private:
    mathai::dao::ChapterDao chapterDao_;
};

} // namespace mathai::services
