#pragma once

#include <drogon/HttpResponse.h>
#include <functional>

#include "clients/AiClient.h"
#include "dao/AiExplanationDao.h"

namespace mathai::services
{

class AiService
{
  public:
    using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

    void explainQuestion(long long questionId, ResponseCallback callback) const;
    void followUpQuestion(long long questionId, const Json::Value &requestBody, ResponseCallback callback) const;

  private:
    std::string buildPrompt(const Json::Value &questionContext) const;
    std::string buildFollowUpPrompt(const Json::Value &questionContext,
                                    const Json::Value &existingExplanation,
                                    const std::string &studentQuestion) const;
    std::string questionTypeLabel(const std::string &type) const;

    mathai::dao::AiExplanationDao aiExplanationDao_;
    mathai::clients::AiClient aiClient_;
};

} // namespace mathai::services
