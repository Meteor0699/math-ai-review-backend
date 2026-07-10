#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <functional>
#include <json/json.h>

#include "dao/QuestionDao.h"

namespace mathai::services
{

class QuestionService
{
  public:
    using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

    void list(const drogon::HttpRequestPtr &request, int page, int pageSize, ResponseCallback callback) const;
    void listAdmin(const drogon::HttpRequestPtr &request, int page, int pageSize, ResponseCallback callback) const;
    void studentDetail(long long id, ResponseCallback callback) const;
    void adminDetail(long long id, ResponseCallback callback) const;
    void answer(long long id, ResponseCallback callback) const;
    void create(const Json::Value &requestBody, ResponseCallback callback) const;
    void update(long long id, const Json::Value &requestBody, ResponseCallback callback) const;
    void disable(long long id, ResponseCallback callback) const;
    void addOptions(long long questionId, const Json::Value &requestBody, ResponseCallback callback) const;
    void replaceOptions(long long questionId, const Json::Value &requestBody, ResponseCallback callback) const;
    void deleteOption(long long optionId, ResponseCallback callback) const;

  private:
    mathai::dao::QuestionDao questionDao_;
};

} // namespace mathai::services
