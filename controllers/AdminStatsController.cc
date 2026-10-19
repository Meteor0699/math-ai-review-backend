#include "controllers/AdminStatsController.h"

#include <drogon/drogon.h>

#include "utils/JsonResponse.h"
#include "utils/MysqlSyncClient.h"

void AdminStatsController::summary(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT "
            "(SELECT COUNT(*) FROM user) AS user_count, "
            "(SELECT COUNT(*) FROM course WHERE status = 1) AS course_count, "
            "(SELECT COUNT(*) FROM chapter WHERE status = 1) AS chapter_count, "
            "(SELECT COUNT(*) FROM knowledge_point) AS knowledge_count, "
            "(SELECT COUNT(*) FROM question WHERE status = 1) AS question_count, "
            "(SELECT COUNT(*) FROM exam_paper WHERE status = 1) AS paper_count, "
            "(SELECT COUNT(*) FROM ai_explanation WHERE status = 'success') AS ai_explanation_count");

        const auto &row = result.rows.front();
        Json::Value data;
        data["users"] = Json::Int64(row.getInt64("user_count"));
        data["courses"] = Json::Int64(row.getInt64("course_count"));
        data["chapters"] = Json::Int64(row.getInt64("chapter_count"));
        data["knowledgePoints"] = Json::Int64(row.getInt64("knowledge_count"));
        data["questions"] = Json::Int64(row.getInt64("question_count"));
        data["papers"] = Json::Int64(row.getInt64("paper_count"));
        data["aiExplanations"] = Json::Int64(row.getInt64("ai_explanation_count"));
        callback(mathai::utils::jsonResponse(200, "success", data));
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Admin statistics database error: " << exception.what();
        callback(mathai::utils::jsonResponse(500, "database error",
                                             Json::Value(Json::objectValue),
                                             drogon::k500InternalServerError));
    }
}
