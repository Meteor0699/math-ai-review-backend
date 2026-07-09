#pragma once

#include <drogon/HttpController.h>

class PaperController : public drogon::HttpController<PaperController>
{
  public:
    METHOD_LIST_BEGIN
    // Public routes
    ADD_METHOD_TO(PaperController::list, "/api/papers", drogon::Get);
    ADD_METHOD_TO(PaperController::list, "/api/v1/papers", drogon::Get);
    ADD_METHOD_TO(PaperController::detail, "/api/papers/{1}", drogon::Get);
    ADD_METHOD_TO(PaperController::detail, "/api/v1/papers/{1}", drogon::Get);
    ADD_METHOD_TO(PaperController::download, "/api/papers/{1}/download", drogon::Get);
    ADD_METHOD_TO(PaperController::download, "/api/v1/papers/{1}/download", drogon::Get);
    // Student personal paper library
    ADD_METHOD_TO(PaperController::list, "/api/my/papers", drogon::Get, "UserAuthFilter");
    ADD_METHOD_TO(PaperController::upload, "/api/my/papers/upload", drogon::Post, "UserAuthFilter");
    // Admin routes
    ADD_METHOD_TO(PaperController::list, "/api/admin/papers", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(PaperController::detail, "/api/admin/papers/{1}", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(PaperController::upload, "/api/admin/papers/upload", drogon::Post, "AdminAuthFilter");
    ADD_METHOD_TO(PaperController::upload, "/api/v1/admin/papers/upload", drogon::Post, "AdminAuthFilter");
    ADD_METHOD_TO(PaperController::update, "/api/admin/papers/{1}", drogon::Put, "AdminAuthFilter");
    ADD_METHOD_TO(PaperController::disable, "/api/admin/papers/{1}", drogon::Delete, "AdminAuthFilter");
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void detail(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long paperId);

    void download(const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  long long paperId);

    void upload(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void update(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long paperId);

    void disable(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                 long long paperId);
};
