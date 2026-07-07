#pragma once

#include <drogon/HttpController.h>

#include "services/AuthService.h"

class AuthController : public drogon::HttpController<AuthController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);
    ADD_METHOD_TO(AuthController::login, "/api/v1/auth/login", drogon::Post);
    ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", drogon::Post);
    ADD_METHOD_TO(AuthController::registerUser, "/api/v1/auth/register", drogon::Post);
    ADD_METHOD_TO(AuthController::me, "/api/v1/auth/me", drogon::Get);
    METHOD_LIST_END

    void login(const drogon::HttpRequestPtr &request,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void registerUser(const drogon::HttpRequestPtr &request,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void me(const drogon::HttpRequestPtr &request,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  private:
    mathai::services::AuthService authService_;
};
