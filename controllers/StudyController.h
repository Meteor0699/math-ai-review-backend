#pragma once

#include <drogon/HttpController.h>

#include "services/StudyService.h"

class StudyController : public drogon::HttpController<StudyController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(StudyController::getState, "/api/questions/{1}/study-state", drogon::Get, "UserAuthFilter");
    ADD_METHOD_TO(StudyController::recordAttempt, "/api/questions/{1}/study-record", drogon::Post, "UserAuthFilter");
    ADD_METHOD_TO(StudyController::addWrong, "/api/questions/{1}/wrong-book", drogon::Post, "UserAuthFilter");
    ADD_METHOD_TO(StudyController::removeWrong, "/api/questions/{1}/wrong-book", drogon::Delete, "UserAuthFilter");
    ADD_METHOD_TO(StudyController::listAttempts, "/api/my/question-records", drogon::Get, "UserAuthFilter");
    ADD_METHOD_TO(StudyController::listWrong, "/api/my/wrong-questions", drogon::Get, "UserAuthFilter");
    METHOD_LIST_END

    void getState(const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  long long questionId);
    void recordAttempt(const drogon::HttpRequestPtr &request,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       long long questionId);
    void addWrong(const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  long long questionId);
    void removeWrong(const drogon::HttpRequestPtr &request,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                     long long questionId);
    void listAttempts(const drogon::HttpRequestPtr &request,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void listWrong(const drogon::HttpRequestPtr &request,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  private:
    mathai::services::StudyService studyService_;
};
