#include "controllers/AiController.h"

void AiController::explainQuestion(const drogon::HttpRequestPtr &,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                   long long questionId)
{
    aiService_.explainQuestion(questionId, std::move(callback));
}

void AiController::followUpQuestion(const drogon::HttpRequestPtr &request,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                    long long questionId)
{
    const auto json = request->getJsonObject();
    if (!json)
    {
        Json::Value empty(Json::objectValue);
        aiService_.followUpQuestion(questionId, empty, std::move(callback));
        return;
    }
    aiService_.followUpQuestion(questionId, *json, std::move(callback));
}
