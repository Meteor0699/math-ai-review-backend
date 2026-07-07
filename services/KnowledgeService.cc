#include "services/KnowledgeService.h"

#include <drogon/drogon.h>

#include <memory>

#include "utils/JsonResponse.h"

namespace mathai::services
{
namespace
{

bool validKnowledgeBody(const Json::Value &body)
{
    return body.isMember("courseId") && body["courseId"].isIntegral() &&
           body.isMember("chapterId") && body["chapterId"].isIntegral() &&
           body.isMember("title") && body["title"].isString() &&
           !body["title"].asString().empty();
}

void databaseError(const std::shared_ptr<KnowledgeService::ResponseCallback> &callback,
                   const std::string &message)
{
    LOG_ERROR << "Knowledge database error: " << message;
    (*callback)(mathai::utils::jsonResponse(500, "database error",
                                            Json::Value(Json::objectValue),
                                            drogon::k500InternalServerError));
}

} // namespace

void KnowledgeService::listActiveByChapter(long long chapterId, int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    knowledgeDao_.listActiveByChapter(
        chapterId,
        page,
        pageSize,
        [cb, chapterId, page, pageSize](Json::Value result) {
            Json::Value data;
            data["chapterId"] = Json::Int64(chapterId);
            data["items"] = result["items"];
            data["total"] = result["total"];
            data["page"] = page;
            data["pageSize"] = pageSize;
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void KnowledgeService::listAll(const drogon::HttpRequestPtr &request, int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    knowledgeDao_.listAll(
        request,
        page,
        pageSize,
        [cb, page, pageSize](Json::Value result) {
            Json::Value data;
            data["items"] = result["items"];
            data["total"] = result["total"];
            data["page"] = page;
            data["pageSize"] = pageSize;
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void KnowledgeService::create(const Json::Value &requestBody, ResponseCallback callback) const
{
    if (!validKnowledgeBody(requestBody))
    {
        callback(mathai::utils::jsonResponse(400, "courseId, chapterId and title are required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    knowledgeDao_.create(
        requestBody,
        [cb](Json::Value data) {
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void KnowledgeService::update(long long id, const Json::Value &requestBody, ResponseCallback callback) const
{
    if (!validKnowledgeBody(requestBody))
    {
        callback(mathai::utils::jsonResponse(400, "courseId, chapterId and title are required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    knowledgeDao_.update(
        id,
        requestBody,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "knowledge point not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void KnowledgeService::disable(long long id, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    knowledgeDao_.disable(
        id,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "knowledge point not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

} // namespace mathai::services
