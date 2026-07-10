#pragma once

#include <drogon/HttpController.h>

#include "services/AiService.h"

class AiController : public drogon::HttpController<AiController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AiController::explainQuestion, "/api/questions/{1}/ai-explanation", drogon::Post, "UserAuthFilter", "RateLimitFilter");
    ADD_METHOD_TO(AiController::explainQuestion, "/api/v1/questions/{1}/ai-explanation", drogon::Post, "UserAuthFilter", "RateLimitFilter");
    ADD_METHOD_TO(AiController::followUpQuestion, "/api/questions/{1}/ai-follow-up", drogon::Post, "UserAuthFilter", "RateLimitFilter");
    ADD_METHOD_TO(AiController::followUpQuestion, "/api/v1/questions/{1}/ai-follow-up", drogon::Post, "UserAuthFilter", "RateLimitFilter");
    METHOD_LIST_END

    void explainQuestion(const drogon::HttpRequestPtr &request,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                         long long questionId);
    void followUpQuestion(const drogon::HttpRequestPtr &request,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                          long long questionId);

  private:
    mathai::services::AiService aiService_;
};
