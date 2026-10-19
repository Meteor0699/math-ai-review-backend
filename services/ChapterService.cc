#include "services/ChapterService.h"

#include <drogon/drogon.h>

#include <memory>

#include "utils/JsonResponse.h"

namespace mathai::services
{
namespace
{

bool validChapterBody(const Json::Value &body)
{
    return body.isMember("courseId") && body["courseId"].isIntegral() &&
           body.isMember("title") && body["title"].isString() &&
           !body["title"].asString().empty();
}

void databaseError(const std::shared_ptr<ChapterService::ResponseCallback> &callback,
                   const std::string &message)
{
    LOG_ERROR << "Chapter database error: " << message;
    (*callback)(mathai::utils::jsonResponse(500, "database error",
                                            Json::Value(Json::objectValue),
                                            drogon::k500InternalServerError));
}

} // namespace

void ChapterService::listActiveByCourse(long long courseId, int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    chapterDao_.listActiveByCourse(
        courseId,
        page,
        pageSize,
        [cb, courseId, page, pageSize](Json::Value result) {
            Json::Value data;
            data["courseId"] = Json::Int64(courseId);
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

void ChapterService::detail(long long id, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    chapterDao_.findActiveById(
        id,
        [cb](Json::Value data) {
            if (data.isNull())
            {
                (*cb)(mathai::utils::jsonResponse(404, "chapter not found",
                                                  Json::Value(Json::objectValue),
                                                  drogon::k404NotFound));
                return;
            }
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void ChapterService::listAll(const drogon::HttpRequestPtr &request, int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    chapterDao_.listAll(
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

void ChapterService::create(const Json::Value &requestBody, ResponseCallback callback) const
{
    if (!validChapterBody(requestBody))
    {
        callback(mathai::utils::jsonResponse(400, "courseId and title are required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    chapterDao_.create(
        requestBody,
        [cb](Json::Value data) {
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void ChapterService::update(long long id, const Json::Value &requestBody, ResponseCallback callback) const
{
    if (!validChapterBody(requestBody))
    {
        callback(mathai::utils::jsonResponse(400, "courseId and title are required",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    chapterDao_.update(
        id,
        requestBody,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "chapter not found",
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

void ChapterService::disable(long long id, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    chapterDao_.disable(
        id,
        [cb](std::uint64_t affectedRows) {
            if (affectedRows == 0)
            {
                (*cb)(mathai::utils::jsonResponse(404, "chapter not found",
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
