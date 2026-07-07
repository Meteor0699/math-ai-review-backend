#pragma once

#include <drogon/HttpController.h>

#include "services/ChapterService.h"

class ChapterController : public drogon::HttpController<ChapterController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ChapterController::listByCourse, "/api/courses/{1}/chapters", drogon::Get);
    ADD_METHOD_TO(ChapterController::listByCourse, "/api/v1/courses/{1}/chapters", drogon::Get);
    ADD_METHOD_TO(ChapterController::detail, "/api/v1/chapters/{1}", drogon::Get);
    ADD_METHOD_TO(ChapterController::listAll, "/api/admin/chapters", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(ChapterController::create, "/api/admin/chapters", drogon::Post, "AdminAuthFilter");
    ADD_METHOD_TO(ChapterController::update, "/api/admin/chapters/{1}", drogon::Put, "AdminAuthFilter");
    ADD_METHOD_TO(ChapterController::disable, "/api/admin/chapters/{1}", drogon::Delete, "AdminAuthFilter");
    METHOD_LIST_END

    void listByCourse(const drogon::HttpRequestPtr &request,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                      long long courseId);

    void listAll(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void detail(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long chapterId);

    void create(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void update(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long chapterId);

    void disable(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                 long long chapterId);

  private:
    mathai::services::ChapterService chapterService_;
};
