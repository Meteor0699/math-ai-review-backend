#include "dao/AiExplanationDao.h"

#include <drogon/drogon.h>

#include "utils/MysqlSyncClient.h"

namespace mathai::dao
{
namespace
{

Json::Value explanationRowToJson(const mathai::utils::mysql::Row &row)
{
    Json::Value data;
    data["id"] = Json::Int64(row.getInt64("id"));
    data["questionId"] = Json::Int64(row.getInt64("question_id"));
    data["modelName"] = row.getString("model_name");
    data["explanation"] = row.getString("explanation");
    data["cached"] = true;
    return data;
}

Json::Value questionContextRowToJson(const mathai::utils::mysql::Row &row)
{
    Json::Value data;
    data["questionId"] = Json::Int64(row.getInt64("id"));
    data["courseName"] = row.getString("course_name");
    data["chapterName"] = row.getString("chapter_name");
    data["questionType"] = row.getString("question_type");
    data["content"] = row.getString("content");
    data["standardAnswer"] = row.getString("standard_answer");
    data["analysis"] = row.getString("normal_explanation");
    return data;
}

} // namespace

void AiExplanationDao::findExisting(long long questionId, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, question_id, model_name, explanation FROM ai_explanation WHERE question_id = " +
            std::to_string(questionId) + " AND status = 'success' LIMIT 1");
        if (result.rows.empty())
        {
            onSuccess(Json::Value(Json::nullValue));
            return;
        }
        onSuccess(explanationRowToJson(result.rows[0]));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void AiExplanationDao::findQuestionContext(long long questionId, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT q.id, q.question_type, q.content, q.standard_answer, q.normal_explanation, "
            "c.name AS course_name, ch.title AS chapter_name FROM question q "
            "LEFT JOIN course c ON q.course_id = c.id LEFT JOIN chapter ch ON q.chapter_id = ch.id "
            "WHERE q.id = " + std::to_string(questionId) + " AND q.status = 1 LIMIT 1");
        if (result.rows.empty())
        {
            onSuccess(Json::Value(Json::nullValue));
            return;
        }
        onSuccess(questionContextRowToJson(result.rows[0]));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void AiExplanationDao::saveSuccess(long long questionId,
                                   const std::string &modelName,
                                   const std::string &prompt,
                                   const std::string &explanation,
                                   JsonCallback onSuccess,
                                   ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "INSERT INTO ai_explanation (question_id, model_name, prompt, explanation, status) VALUES (" +
            std::to_string(questionId) + ", " + mathai::utils::mysql::quote(modelName) + ", " +
            mathai::utils::mysql::quote(prompt) + ", " + mathai::utils::mysql::quote(explanation) + ", 'success')");
        Json::Value data;
        data["id"] = Json::UInt64(result.insertId);
        data["questionId"] = Json::Int64(questionId);
        data["modelName"] = modelName;
        data["explanation"] = explanation;
        data["cached"] = false;
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

} // namespace mathai::dao
