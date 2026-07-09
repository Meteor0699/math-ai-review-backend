#pragma once

#include <drogon/HttpResponse.h>
#include <functional>
#include <json/json.h>

#include "dao/StudyDao.h"

namespace mathai::services
{

class StudyService
{
  public:
    using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

    void getState(long long userId, long long questionId, ResponseCallback callback) const;
    void recordAttempt(long long userId,
                       long long questionId,
                       const Json::Value &requestBody,
                       ResponseCallback callback) const;
    void listAttempts(long long userId, int page, int pageSize, ResponseCallback callback) const;
    void addWrong(long long userId,
                  long long questionId,
                  const Json::Value &requestBody,
                  ResponseCallback callback) const;
    void removeWrong(long long userId, long long questionId, ResponseCallback callback) const;
    void listWrong(long long userId, int page, int pageSize, ResponseCallback callback) const;

  private:
    mathai::dao::StudyDao studyDao_;
};

} // namespace mathai::services
