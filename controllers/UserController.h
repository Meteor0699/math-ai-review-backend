#pragma once

#include <drogon/HttpController.h>

#include "dao/UserDao.h"

class UserController : public drogon::HttpController<UserController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::list, "/api/admin/users", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(UserController::create, "/api/admin/users", drogon::Post, "AdminAuthFilter");
    ADD_METHOD_TO(UserController::update, "/api/admin/users/{1}", drogon::Put, "AdminAuthFilter");
    ADD_METHOD_TO(UserController::disable, "/api/admin/users/{1}", drogon::Delete, "AdminAuthFilter");
    ADD_METHOD_TO(UserController::resetPassword, "/api/admin/users/{1}/password", drogon::Put, "AdminAuthFilter");
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void create(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void update(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long userId);

    void disable(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                 long long userId);

    void resetPassword(const drogon::HttpRequestPtr &request,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       long long userId);

  private:
    mathai::dao::UserDao userDao_;
};
