#pragma once

#include <drogon/HttpController.h>

#include "services/AgentService.h"

class AgentController : public drogon::HttpController<AgentController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AgentController::getSession, "/api/questions/{1}/agent/session", drogon::Get, "UserAuthFilter");
    ADD_METHOD_TO(AgentController::getSession, "/api/v1/questions/{1}/agent/session", drogon::Get, "UserAuthFilter");
    ADD_METHOD_TO(AgentController::chat, "/api/questions/{1}/agent/chat", drogon::Post, "UserAuthFilter");
    ADD_METHOD_TO(AgentController::chat, "/api/v1/questions/{1}/agent/chat", drogon::Post, "UserAuthFilter");
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
