#pragma once

#include <drogon/HttpController.h>

#include "services/QuestionService.h"

class QuestionController : public drogon::HttpController<QuestionController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(QuestionController::list, "/api/questions", drogon::Get);
    ADD_METHOD_TO(QuestionController::list, "/api/v1/questions", drogon::Get);
    ADD_METHOD_TO(QuestionController::listAdmin, "/api/admin/questions", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(QuestionController::detail, "/api/questions/{1}", drogon::Get);
    ADD_METHOD_TO(QuestionController::detail, "/api/v1/questions/{1}", drogon::Get);
    ADD_METHOD_TO(QuestionController::answer, "/api/questions/{1}/answer", drogon::Get, "UserAuthFilter");
    ADD_METHOD_TO(QuestionController::answer, "/api/v1/questions/{1}/answer", drogon::Get, "UserAuthFilter");
    ADD_METHOD_TO(QuestionController::adminDetail, "/api/admin/questions/{1}", drogon::Get, "AdminAuthFilter");
    ADD_METHOD_TO(QuestionController::create, "/api/admin/questions", drogon::Post, "AdminAuthFilter");
    ADD_METHOD_TO(QuestionController::update, "/api/admin/questions/{1}", drogon::Put, "AdminAuthFilter");
    ADD_METHOD_TO(QuestionController::disable, "/api/admin/questions/{1}", drogon::Delete, "AdminAuthFilter");
    ADD_METHOD_TO(QuestionController::addOptions, "/api/admin/questions/{1}/options", drogon::Post, "AdminAuthFilter");
    ADD_METHOD_TO(QuestionController::replaceOptions, "/api/admin/questions/{1}/options", drogon::Put, "AdminAuthFilter");
    ADD_METHOD_TO(QuestionController::deleteOption, "/api/admin/questions/{1}/options/{2}", drogon::Delete, "AdminAuthFilter");
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr &request,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void listAdmin(const drogon::HttpRequestPtr &request,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void detail(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long questionId);

    void answer(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long questionId);

    void adminDetail(const drogon::HttpRequestPtr &request,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                     long long questionId);

    void create(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void update(const drogon::HttpRequestPtr &request,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                long long questionId);

    void disable(const drogon::HttpRequestPtr &request,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                 long long questionId);

    void addOptions(const drogon::HttpRequestPtr &request,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                    long long questionId);

    void replaceOptions(const drogon::HttpRequestPtr &request,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        long long questionId);

    void deleteOption(const drogon::HttpRequestPtr &request,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                      long long questionId,
                      long long optionId);

  private:
    mathai::services::QuestionService questionService_;
};
