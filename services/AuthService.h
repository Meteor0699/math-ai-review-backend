#pragma once

#include <drogon/HttpResponse.h>
#include <functional>
#include <json/json.h>
#include <string>

#include "dao/UserDao.h"

namespace mathai::services
{

class AuthService
{
  public:
    using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

    void login(const std::string &username,
               const std::string &password,
               ResponseCallback callback) const;

    void registerUser(const Json::Value &requestBody,
                      ResponseCallback callback) const;

  private:
    mathai::dao::UserDao userDao_;
};

} // namespace mathai::services
