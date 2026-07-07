#include "dao/KnowledgeDao.h"

#include <drogon/drogon.h>

#include <sstream>

#include "utils/MysqlSyncClient.h"

namespace mathai::dao
{
namespace
{

Json::Value knowledgeRowToJson(const mathai::utils::mysql::Row &row)
{
    Json::Value item;
    item["id"] = Json::Int64(row.getInt64("id"));
    item["courseId"] = Json::Int64(row.getInt64("course_id"));
    item["chapterId"] = Json::Int64(row.getInt64("chapter_id"));
    item["title"] = row.getString("title");
    item["coreContent"] = row.getString("core_content");
    item["formulas"] = row.getString("formulas");
    item["typicalQuestions"] = row.getString("typical_questions");
    item["commonMistakes"] = row.getString("common_mistakes");
    item["examFrequency"] = row.getString("exam_frequency");
    item["reviewAdvice"] = row.getString("review_advice");
    item["sortOrder"] = row.getInt("sort_order");
    item["status"] = row.getInt("status");
    return item;
}

std::string textField(const Json::Value &value, const char *name)
{
    return mathai::utils::mysql::quote(value.get(name, "").asString());
}

} // namespace

void KnowledgeDao::listActiveByChapter(long long chapterId,
                                       int page,
                                       int pageSize,
                                       JsonCallback onSuccess,
                                       ErrorCallback onError) const
{
    try
    {
        const auto countResult = mathai::utils::mysql::execute(
            "SELECT COUNT(*) AS total FROM knowledge_point WHERE chapter_id = " + std::to_string(chapterId) + " AND status = 1");
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, course_id, chapter_id, title, core_content, formulas, typical_questions, "
            "common_mistakes, exam_frequency, review_advice, sort_order, status "
            "FROM knowledge_point WHERE chapter_id = " + std::to_string(chapterId) +
            " AND status = 1 ORDER BY sort_order ASC, id ASC LIMIT " + std::to_string(pageSize) +
            " OFFSET " + std::to_string(offset));

        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows)
        {
            items.append(knowledgeRowToJson(row));
        }
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

void KnowledgeDao::listAll(const drogon::HttpRequestPtr &request,
                           int page, int pageSize,
                           JsonCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        std::ostringstream whereClause;
        whereClause << " WHERE kp.status = 1";
        const auto courseId = request->getParameter("courseId");
        if (!courseId.empty()) whereClause << " AND kp.course_id = " << std::stoll(courseId);
        const auto chapterId = request->getParameter("chapterId");
        if (!chapterId.empty()) whereClause << " AND kp.chapter_id = " << std::stoll(chapterId);

        const auto whereStr = whereClause.str();
        const auto offset = static_cast<int>((page - 1) * pageSize);
        const auto countResult = mathai::utils::mysql::execute("SELECT COUNT(*) AS total FROM knowledge_point kp" + whereStr);
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto result = mathai::utils::mysql::execute(
            "SELECT kp.id, kp.course_id, kp.chapter_id, kp.title, kp.core_content, kp.formulas, "
            "kp.typical_questions, kp.common_mistakes, kp.exam_frequency, kp.review_advice, kp.sort_order, kp.status, "
            "c.name AS course_name, ch.title AS chapter_name FROM knowledge_point kp "
            "LEFT JOIN course c ON kp.course_id = c.id LEFT JOIN chapter ch ON kp.chapter_id = ch.id" +
            whereStr + " ORDER BY kp.sort_order ASC, kp.id ASC LIMIT " + std::to_string(pageSize) +
            " OFFSET " + std::to_string(offset));

        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows)
        {
            auto item = knowledgeRowToJson(row);
            item["courseName"] = row.getString("course_name");
            item["chapterName"] = row.getString("chapter_name");
            items.append(item);
        }
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

void KnowledgeDao::create(const Json::Value &knowledgePoint,
                          JsonCallback onSuccess,
                          ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "INSERT INTO knowledge_point (course_id, chapter_id, title, core_content, formulas, typical_questions, "
            "common_mistakes, exam_frequency, review_advice, sort_order, status) VALUES (" +
            std::to_string(knowledgePoint["courseId"].asInt64()) + ", " +
            std::to_string(knowledgePoint["chapterId"].asInt64()) + ", " +
            mathai::utils::mysql::quote(knowledgePoint["title"].asString()) + ", " +
            textField(knowledgePoint, "coreContent") + ", " + textField(knowledgePoint, "formulas") + ", " +
            textField(knowledgePoint, "typicalQuestions") + ", " + textField(knowledgePoint, "commonMistakes") + ", " +
            textField(knowledgePoint, "examFrequency") + ", " + textField(knowledgePoint, "reviewAdvice") + ", " +
            std::to_string(knowledgePoint.get("sortOrder", 0).asInt()) + ", " +
            std::to_string(knowledgePoint.get("status", 1).asInt()) + ")");
        Json::Value data;
        data["id"] = Json::UInt64(result.insertId);
        onSuccess(data);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void KnowledgeDao::update(long long id,
                          const Json::Value &knowledgePoint,
                          CountCallback onSuccess,
                          ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "UPDATE knowledge_point SET course_id = " + std::to_string(knowledgePoint["courseId"].asInt64()) +
            ", chapter_id = " + std::to_string(knowledgePoint["chapterId"].asInt64()) +
            ", title = " + mathai::utils::mysql::quote(knowledgePoint["title"].asString()) +
            ", core_content = " + textField(knowledgePoint, "coreContent") +
            ", formulas = " + textField(knowledgePoint, "formulas") +
            ", typical_questions = " + textField(knowledgePoint, "typicalQuestions") +
            ", common_mistakes = " + textField(knowledgePoint, "commonMistakes") +
            ", exam_frequency = " + textField(knowledgePoint, "examFrequency") +
            ", review_advice = " + textField(knowledgePoint, "reviewAdvice") +
            ", sort_order = " + std::to_string(knowledgePoint.get("sortOrder", 0).asInt()) +
            ", status = " + std::to_string(knowledgePoint.get("status", 1).asInt()) +
            " WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void KnowledgeDao::disable(long long id, CountCallback onSuccess, ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute("UPDATE knowledge_point SET status = 0 WHERE id = " + std::to_string(id));
        onSuccess(result.affectedRows);
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

} // namespace mathai::dao
