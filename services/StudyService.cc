#include "services/StudyService.h"

#include <drogon/drogon.h>

#include <memory>
#include <unordered_set>

#include "utils/JsonResponse.h"

namespace mathai::services
{
namespace
{

void databaseError(const std::shared_ptr<StudyService::ResponseCallback> &callback,
                   const std::string &message)
{
    LOG_ERROR << "Study database error: " << message;
    (*callback)(mathai::utils::jsonResponse(500, "database error",
                                            Json::Value(Json::objectValue),
                                            drogon::k500InternalServerError));
}

std::string stringField(const Json::Value &body, const char *field)
{
    if (!body.isMember(field) || body[field].isNull() || !body[field].isString())
    {
        return "";
    }
    return body[field].asString();
}

bool validStatus(const std::string &status)
{
    static const std::unordered_set<std::string> allowed = {"viewed", "mastered", "wrong"};
    return allowed.find(status) != allowed.end();
}

} // namespace

void StudyService::getState(long long userId, long long questionId, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    studyDao_.findState(
        userId,
        questionId,
        [cb](Json::Value data) {
            if (data.isNull())
            {
                (*cb)(mathai::utils::jsonResponse(404, "question not found",
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

void StudyService::recordAttempt(long long userId,
                                 long long questionId,
                                 const Json::Value &requestBody,
                                 ResponseCallback callback) const
{
    auto status = stringField(requestBody, "status");
    if (status.empty())
    {
        status = "viewed";
    }
    if (!validStatus(status))
    {
        callback(mathai::utils::jsonResponse(400, "invalid study status",
                                             Json::Value(Json::objectValue),
                                             drogon::k400BadRequest));
        return;
    }

    const auto note = stringField(requestBody, "note");
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    studyDao_.upsertAttempt(
        userId,
        questionId,
        status,
        note,
        [cb](Json::Value data) {
            if (data.isNull())
            {
                (*cb)(mathai::utils::jsonResponse(404, "question not found",
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

void StudyService::listAttempts(long long userId, int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    studyDao_.listAttempts(
        userId,
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

void StudyService::addWrong(long long userId,
                            long long questionId,
                            const Json::Value &requestBody,
                            ResponseCallback callback) const
{
    const auto note = stringField(requestBody, "note");
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    studyDao_.addWrong(
        userId,
        questionId,
        note,
        [cb](Json::Value data) {
            if (data.isNull())
            {
                (*cb)(mathai::utils::jsonResponse(404, "question not found",
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

void StudyService::removeWrong(long long userId, long long questionId, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    studyDao_.removeWrong(
        userId,
        questionId,
        [cb](std::uint64_t) {
            (*cb)(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
        },
        [cb](const std::string &message) {
            databaseError(cb, message);
        });
}

void StudyService::listWrong(long long userId, int page, int pageSize, ResponseCallback callback) const
{
    auto cb = std::make_shared<ResponseCallback>(std::move(callback));
    studyDao_.listWrong(
        userId,
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

} // namespace mathai::services
