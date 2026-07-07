#pragma once

#include <drogon/HttpController.h>

#include "services/KnowledgeService.h"

class KnowledgeController : public drogon::HttpController<KnowledgeController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(KnowledgeController::listByChapter, "/api/chapters/{1}/knowledge-points", drogon::Get);
    ADD_METHOD_TO(KnowledgeController::listByChapter, "/api/v1/chapters/{1}/knowledge-points", drogon::Get);
    ADD_METHOD_TO(KnowledgeController::listAll, "/api/admin/knowledge-points", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(KnowledgeController::create, "/api/admin/knowledge-points", drogon::Post, "AdminAuthFilter");
    ADD_METHOD_TO(KnowledgeController::update, "/api/admin/knowledge-points/{1}", drogon::Put, "AdminAuthFilter");
    ADD_METHOD_TO(KnowledgeController::disable, "/api/admin/knowledge-points/{1}", drogon::Delete, "AdminAuthFilter");
    METHOD_LIST_END

    void listByChapter(const drogon::HttpRequestPtr &request,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       long long chapterId);

    void listAll(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void create(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void update(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long knowledgeId);

    void disable(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                 long long knowledgeId);

  private:
    mathai::services::KnowledgeService knowledgeService_;
};
