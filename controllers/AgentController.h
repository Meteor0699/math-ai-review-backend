#pragma once

#include <drogon/HttpController.h>

#include "services/AgentService.h"

class AgentController : public drogon::HttpController<AgentController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AgentController::getSession, "/api/questions/{1}/agent/session", drogon::Get, "UserAuthFilter");
    ADD_METHOD_TO(AgentController::getSession, "/api/v1/questions/{1}/agent/session", drogon::Get, "UserAuthFilter");
    // These turns invoke the model (and the tools), so they carry the same
    // per-user rate limit as the existing AI routes in AiController.h.
    ADD_METHOD_TO(AgentController::chat, "/api/questions/{1}/agent/chat", drogon::Post, "UserAuthFilter", "RateLimitFilter");
    ADD_METHOD_TO(AgentController::chat, "/api/v1/questions/{1}/agent/chat", drogon::Post, "UserAuthFilter", "RateLimitFilter");
    METHOD_LIST_END

    void getSession(const drogon::HttpRequestPtr &request,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                    long long questionId);
    void chat(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              long long questionId);

  private:
    mathai::services::AgentService agentService_;
};
