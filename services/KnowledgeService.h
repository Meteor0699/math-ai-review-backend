#pragma once

#include <drogon/HttpResponse.h>
#include <functional>
#include <json/json.h>

#include "dao/KnowledgeDao.h"

namespace mathai::services
{

class KnowledgeService
{
  public:
    using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

    void listActiveByChapter(long long chapterId, int page, int pageSize, ResponseCallback callback) const;
    void listAll(const drogon::HttpRequestPtr &request, int page, int pageSize, ResponseCallback callback) const;
    void create(const Json::Value &requestBody, ResponseCallback callback) const;
    void update(long long id, const Json::Value &requestBody, ResponseCallback callback) const;
    void disable(long long id, ResponseCallback callback) const;

  private:
    mathai::dao::KnowledgeDao knowledgeDao_;
};

} // namespace mathai::services
