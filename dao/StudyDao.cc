#include "dao/StudyDao.h"

#include <sstream>

#include "utils/MysqlSyncClient.h"

namespace mathai::dao
{
namespace
{

std::string quote(const std::string &value) { return mathai::utils::mysql::quote(value); }

std::string nullableNote(const std::string &value)
{
    return value.empty() ? "NULL" : quote(value);
}

std::string questionTypeLabel(const std::string &type)
{
    if (type == "single_choice") return u8"\u5355\u9009\u9898";
    if (type == "blank") return u8"\u586b\u7a7a\u9898";
    if (type == "calculation") return u8"\u8ba1\u7b97\u9898";
    if (type == "solution") return u8"\u89e3\u7b54\u9898";
    if (type == "proof") return u8"\u8bc1\u660e\u9898";
    return type;
}

std::string difficultyLabel(const std::string &difficulty)
{
    if (difficulty == "basic") return u8"\u57fa\u7840";
    if (difficulty == "medium") return u8"\u4e2d\u7b49";
    if (difficulty == "hard") return u8"\u8f83\u96be";
    return difficulty;
}

Json::Value questionRowToJson(const mathai::utils::mysql::Row &row)
{
    const auto questionType = row.getString("question_type");
    const auto difficulty = row.getString("difficulty");

    Json::Value item;
    item["id"] = Json::Int64(row.getInt64("question_id"));
    item["questionId"] = Json::Int64(row.getInt64("question_id"));
    item["courseId"] = Json::Int64(row.getInt64("course_id"));
    item["chapterId"] = row.isNull("chapter_id") ? Json::Value(Json::nullValue) : Json::Value(Json::Int64(row.getInt64("chapter_id")));
    item["title"] = row.getString("title");
    item["content"] = row.getString("content");
    item["questionType"] = questionType;
    item["questionTypeLabel"] = questionTypeLabel(questionType);
    item["difficulty"] = difficulty;
    item["difficultyLabel"] = difficultyLabel(difficulty);
    item["source"] = row.getString("source");
    item["year"] = row.isNull("source_year") ? Json::Value(Json::nullValue) : Json::Value(row.getInt("source_year"));
    item["courseName"] = row.getString("course_name");
    item["chapterName"] = row.getString("chapter_name");
    return item;
}

Json::Value attemptRowToJson(const mathai::utils::mysql::Row &row)
{
    auto item = questionRowToJson(row);
    item["recordId"] = Json::Int64(row.getInt64("record_id"));
    item["status"] = row.getString("record_status");
    item["note"] = row.getString("record_note");
    item["attemptCount"] = row.getInt("attempt_count");
    item["firstViewedAt"] = row.getString("first_viewed_at");
    item["lastAttemptAt"] = row.getString("last_attempt_at");
    item["isWrong"] = row.getInt("is_wrong") == 1;
    return item;
}

Json::Value wrongRowToJson(const mathai::utils::mysql::Row &row)
{
    auto item = questionRowToJson(row);
    item["wrongId"] = Json::Int64(row.getInt64("wrong_id"));
    item["note"] = row.getString("wrong_note");
    item["addedAt"] = row.getString("added_at");
    return item;
}

} // namespace

void StudyDao::findState(long long userId, long long questionId, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT qa.id AS record_id, qa.status AS record_status, qa.note AS record_note, qa.attempt_count, "
            "qa.first_viewed_at, qa.last_attempt_at, "
            "CASE WHEN wq.id IS NULL THEN 0 ELSE 1 END AS is_wrong "
            "FROM question q "
            "LEFT JOIN question_attempt qa ON qa.question_id = q.id AND qa.user_id = " + std::to_string(userId) + " "
            "LEFT JOIN wrong_question wq ON wq.question_id = q.id AND wq.user_id = " + std::to_string(userId) + " AND wq.status = 1 "
            "WHERE q.id = " + std::to_string(questionId) + " AND q.status = 1 LIMIT 1");
        if (result.rows.empty())
        {
            onSuccess(Json::Value(Json::nullValue));
            return;
        }

        const auto &row = result.rows[0];
        Json::Value data;
        data["questionId"] = Json::Int64(questionId);
        data["hasRecord"] = !row.isNull("record_id");
        data["recordId"] = row.isNull("record_id") ? Json::Value(Json::nullValue) : Json::Value(Json::Int64(row.getInt64("record_id")));
        data["status"] = row.getString("record_status", "none");
        data["note"] = row.getString("record_note");
        data["attemptCount"] = row.getInt("attempt_count");
        data["firstViewedAt"] = row.getString("first_viewed_at");
        data["lastAttemptAt"] = row.getString("last_attempt_at");
        data["isWrong"] = row.getInt("is_wrong") == 1;
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void StudyDao::upsertAttempt(long long userId,
                             long long questionId,
                             const std::string &status,
                             const std::string &note,
                             JsonCallback onSuccess,
                             ErrorCallback onError) const
{
    try
    {
        mathai::utils::mysql::execute(
            "INSERT INTO question_attempt (user_id, question_id, status, note, attempt_count) VALUES (" +
            std::to_string(userId) + ", " + std::to_string(questionId) + ", " + quote(status) + ", " + nullableNote(note) + ", 1) "
            "ON DUPLICATE KEY UPDATE status = VALUES(status), "
            "note = COALESCE(VALUES(note), note), "
            "attempt_count = attempt_count + 1, "
            "last_attempt_at = CURRENT_TIMESTAMP");
        findState(userId, questionId, std::move(onSuccess), std::move(onError));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void StudyDao::listAttempts(long long userId, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto countResult = mathai::utils::mysql::execute(
            "SELECT COUNT(*) AS total FROM question_attempt WHERE user_id = " + std::to_string(userId));
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto result = mathai::utils::mysql::execute(
            "SELECT qa.id AS record_id, qa.status AS record_status, qa.note AS record_note, qa.attempt_count, "
            "qa.first_viewed_at, qa.last_attempt_at, "
            "CASE WHEN wq.id IS NULL THEN 0 ELSE 1 END AS is_wrong, "
            "q.id AS question_id, q.course_id, q.chapter_id, q.title, q.content, q.question_type, q.difficulty, q.source, q.source_year, "
            "c.name AS course_name, ch.title AS chapter_name "
            "FROM question_attempt qa "
            "JOIN question q ON qa.question_id = q.id "
            "LEFT JOIN course c ON q.course_id = c.id "
            "LEFT JOIN chapter ch ON q.chapter_id = ch.id "
            "LEFT JOIN wrong_question wq ON wq.question_id = q.id AND wq.user_id = qa.user_id AND wq.status = 1 "
            "WHERE qa.user_id = " + std::to_string(userId) + " AND q.status = 1 "
            "ORDER BY qa.last_attempt_at DESC, qa.id DESC LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset));

        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows) items.append(attemptRowToJson(row));
        Json::Value data;
        data["items"] = items;
        data["total"] = total;
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void StudyDao::addWrong(long long userId,
                        long long questionId,
                        const std::string &note,
                        JsonCallback onSuccess,
                        ErrorCallback onError) const
{
    try
    {
        mathai::utils::mysql::execute(
            "INSERT INTO wrong_question (user_id, question_id, note, status, added_at, removed_at) VALUES (" +
            std::to_string(userId) + ", " + std::to_string(questionId) + ", " + nullableNote(note) + ", 1, CURRENT_TIMESTAMP, NULL) "
            "ON DUPLICATE KEY UPDATE note = VALUES(note), status = 1, added_at = CURRENT_TIMESTAMP, removed_at = NULL");
        upsertAttempt(userId, questionId, "wrong", note, std::move(onSuccess), std::move(onError));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void StudyDao::removeWrong(long long userId, long long questionId, CountCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "UPDATE wrong_question SET status = 0, removed_at = CURRENT_TIMESTAMP "
            "WHERE user_id = " + std::to_string(userId) + " AND question_id = " + std::to_string(questionId) + " AND status = 1");
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void StudyDao::listWrong(long long userId, int page, int pageSize, JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto countResult = mathai::utils::mysql::execute(
            "SELECT COUNT(*) AS total FROM wrong_question wq JOIN question q ON wq.question_id = q.id "
            "WHERE wq.user_id = " + std::to_string(userId) + " AND wq.status = 1 AND q.status = 1");
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto result = mathai::utils::mysql::execute(
            "SELECT wq.id AS wrong_id, wq.note AS wrong_note, wq.added_at, "
            "q.id AS question_id, q.course_id, q.chapter_id, q.title, q.content, q.question_type, q.difficulty, q.source, q.source_year, "
            "c.name AS course_name, ch.title AS chapter_name "
            "FROM wrong_question wq "
            "JOIN question q ON wq.question_id = q.id "
            "LEFT JOIN course c ON q.course_id = c.id "
            "LEFT JOIN chapter ch ON q.chapter_id = ch.id "
            "WHERE wq.user_id = " + std::to_string(userId) + " AND wq.status = 1 AND q.status = 1 "
            "ORDER BY wq.added_at DESC, wq.id DESC LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset));

        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows) items.append(wrongRowToJson(row));
        Json::Value data;
        data["items"] = items;
        data["total"] = total;
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

} // namespace mathai::dao
