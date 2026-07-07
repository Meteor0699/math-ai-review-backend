#pragma once

#include <drogon/HttpController.h>

#include "services/CourseService.h"

class CourseController : public drogon::HttpController<CourseController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CourseController::list, "/api/courses", drogon::Get);
    ADD_METHOD_TO(CourseController::list, "/api/v1/courses", drogon::Get);
    ADD_METHOD_TO(CourseController::list, "/api/admin/courses", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(CourseController::detail, "/api/courses/{1}", drogon::Get);
    ADD_METHOD_TO(CourseController::detail, "/api/v1/courses/{1}", drogon::Get);
    ADD_METHOD_TO(CourseController::create, "/api/admin/courses", drogon::Post, "AdminAuthFilter");
    ADD_METHOD_TO(CourseController::update, "/api/admin/courses/{1}", drogon::Put, "AdminAuthFilter");
    ADD_METHOD_TO(CourseController::disable, "/api/admin/courses/{1}", drogon::Delete, "AdminAuthFilter");
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void detail(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long courseId);

    void create(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void update(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long courseId);

    void disable(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                 long long courseId);

  private:
    mathai::services::CourseService courseService_;
};
