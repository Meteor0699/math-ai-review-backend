﻿﻿﻿#include "controllers/PaperController.h"

#include <drogon/drogon.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "utils/JsonResponse.h"
#include "utils/MysqlSyncClient.h"

namespace
{

std::string quote(const std::string &value) { return mathai::utils::mysql::quote(value); }

Json::Value paperRowToJson(const mathai::utils::mysql::Row &row)
{
    Json::Value item;
    const auto id = row.getInt64("id");
    item["id"] = Json::Int64(id);
    item["courseId"] = Json::Int64(row.getInt64("course_id"));
    item["title"] = row.getString("title");
    item["examYear"] = row.isNull("exam_year") ? Json::Value(Json::nullValue) : Json::Value(row.getInt("exam_year"));
    item["semester"] = row.getString("semester");
    item["originalFilename"] = row.getString("original_filename");
    item["fileName"] = row.getString("original_filename");
    item["storedFilename"] = row.getString("stored_filename");
    item["filePath"] = row.getString("file_path");
    item["fileUrl"] = "/api/papers/" + std::to_string(id) + "/download";
    item["fileSize"] = Json::Int64(row.getInt64("file_size"));
    item["fileType"] = row.getString("file_type");
    item["splitStatus"] = row.getString("split_status");
    item["status"] = row.getInt("status");
    item["courseName"] = row.getString("course_name");
    return item;
}

std::string generateStoredFilename(const std::string &originalFilename)
{
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    auto dotPos = originalFilename.find_last_of('.');
    std::string ext = (dotPos != std::string::npos) ? originalFilename.substr(dotPos) : "";
    return std::to_string(timestamp) + ext;
}

std::string lowerExt(std::string ext)
{
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return ext;
}

std::string fileContentType(const std::string &fileType)
{
    const auto ext = lowerExt(fileType);
    if (ext == "pdf" || ext == ".pdf") return "application/pdf";
    if (ext == "doc" || ext == ".doc") return "application/msword";
    if (ext == "docx" || ext == ".docx") return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (ext == "txt" || ext == ".txt") return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

} // namespace

void PaperController::list(const drogon::HttpRequestPtr &request,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    try
    {
        const auto p = mathai::utils::parsePagination(request);
        const auto offset = static_cast<int>((p.page - 1) * p.pageSize);
        std::ostringstream whereClause;
        whereClause << " WHERE ep.status = 1";
        const auto courseId = request->getParameter("courseId");
        if (!courseId.empty()) whereClause << " AND ep.course_id = " << std::stoll(courseId);
        const auto examYear = request->getParameter("examYear");
        const auto year = examYear.empty() ? request->getParameter("year") : examYear;
        if (!year.empty()) whereClause << " AND ep.exam_year = " << std::stoi(year);
        const auto whereStr = whereClause.str();
        const auto countResult = mathai::utils::mysql::execute("SELECT COUNT(*) AS total FROM exam_paper ep" + whereStr);
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto result = mathai::utils::mysql::execute(
            "SELECT ep.id, ep.course_id, ep.title, ep.exam_year, ep.semester, ep.original_filename, ep.stored_filename, "
            "ep.file_path, ep.file_size, ep.file_type, ep.split_status, ep.status, c.name AS course_name "
            "FROM exam_paper ep LEFT JOIN course c ON ep.course_id = c.id" +
            whereStr + " ORDER BY ep.exam_year DESC, ep.id DESC LIMIT " + std::to_string(p.pageSize) +
            " OFFSET " + std::to_string(offset));
        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows) items.append(paperRowToJson(row));
        Json::Value data;
        data["items"] = items;
        data["total"] = total;
        data["page"] = p.page;
        data["pageSize"] = p.pageSize;
        callback(mathai::utils::jsonResponse(200, "success", data));
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Paper list query error: " << exception.what();
        callback(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue), drogon::k500InternalServerError));
    }
}

void PaperController::detail(const drogon::HttpRequestPtr &,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             long long paperId)
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT ep.id, ep.course_id, ep.title, ep.exam_year, ep.semester, ep.original_filename, ep.stored_filename, "
            "ep.file_path, ep.file_size, ep.file_type, ep.split_status, ep.status, c.name AS course_name "
            "FROM exam_paper ep LEFT JOIN course c ON ep.course_id = c.id WHERE ep.id = " + std::to_string(paperId));
        if (result.rows.empty())
        {
            callback(mathai::utils::jsonResponse(404, "paper not found", Json::Value(Json::objectValue), drogon::k404NotFound));
            return;
        }
        callback(mathai::utils::jsonResponse(200, "success", paperRowToJson(result.rows[0])));
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Paper detail query error: " << exception.what();
        callback(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue), drogon::k500InternalServerError));
    }
}

void PaperController::download(const drogon::HttpRequestPtr &,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               long long paperId)
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, original_filename, file_path, file_type, status FROM exam_paper WHERE id = " +
            std::to_string(paperId) + " AND status = 1");
        if (result.rows.empty())
        {
            callback(mathai::utils::jsonResponse(404, "paper not found", Json::Value(Json::objectValue), drogon::k404NotFound));
            return;
        }

        const auto &row = result.rows[0];
        const auto filePath = row.getString("file_path");
        if (filePath.empty() || !std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath))
        {
            callback(mathai::utils::jsonResponse(404, "paper file not found", Json::Value(Json::objectValue), drogon::k404NotFound));
            return;
        }

        auto response = drogon::HttpResponse::newFileResponse(filePath);
        response->setContentTypeString(fileContentType(row.getString("file_type")));
        response->addHeader("Content-Disposition", "inline; filename=\"" + row.getString("original_filename") + "\"");
        callback(response);
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Paper download error: " << exception.what();
        callback(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue), drogon::k500InternalServerError));
    }
}

void PaperController::upload(const drogon::HttpRequestPtr &request,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    drogon::MultiPartParser parser;
    if (parser.parse(request) != 0)
    {
        callback(mathai::utils::jsonResponse(400, "failed to parse multipart", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }

    const auto &files = parser.getFiles();
    if (files.empty())
    {
        callback(mathai::utils::jsonResponse(400, "no file uploaded", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }

    const auto &file = files[0];
    const auto originalFilename = file.getFileName();
    const auto ext = lowerExt(std::string(file.getFileExtension()));
    static const std::unordered_set<std::string> allowedExts{"pdf", "doc", "docx", "txt", ".pdf", ".doc", ".docx", ".txt"};
    if (allowedExts.find(ext) == allowedExts.end())
    {
        callback(mathai::utils::jsonResponse(400, "unsupported file type", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }
    constexpr size_t maxFileSize = 20 * 1024 * 1024;
    if (file.fileLength() > maxFileSize)
    {
        callback(mathai::utils::jsonResponse(400, "file too large", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }

    const auto storedFilename = generateStoredFilename(originalFilename);
    const auto &params = parser.getParameters();
    const auto findParam = [&params](const std::string &name) -> std::string {
        const auto it = params.find(name);
        return it == params.end() ? "" : it->second;
    };
    const auto courseId = findParam("courseId");
    const auto titleParam = findParam("title");
    const auto paperNameParam = findParam("paperName");
    const auto title = titleParam.empty() ? paperNameParam : titleParam;
    const auto examYearParam = findParam("examYear");
    const auto yearParam = findParam("year");
    const auto year = examYearParam.empty() ? yearParam : examYearParam;
    if (courseId.empty() || title.empty())
    {
        callback(mathai::utils::jsonResponse(400, "courseId and title are required", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }

    const auto uploadDir = drogon::app().getCustomConfig().get("upload_dir", "./uploads/papers").asString();
    auto filePath = uploadDir + "/" + storedFilename;
    if (!std::filesystem::exists(uploadDir)) std::filesystem::create_directories(uploadDir);
    file.saveAs(filePath);

    try
    {
        const auto result = mathai::utils::mysql::execute(
            "INSERT INTO exam_paper (course_id, title, exam_year, original_filename, stored_filename, file_path, file_size, file_type) VALUES (" +
            std::to_string(std::stoll(courseId)) + ", " + quote(title) + ", " +
            (year.empty() ? "NULL" : std::to_string(std::stoi(year))) + ", " + quote(originalFilename) + ", " +
            quote(storedFilename) + ", " + quote(filePath) + ", " + std::to_string(file.fileLength()) + ", " + quote(ext) + ")");
        Json::Value data;
        data["id"] = Json::UInt64(result.insertId);
        callback(mathai::utils::jsonResponse(200, "success", data));
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Paper upload error: " << exception.what();
        std::filesystem::remove(filePath);
        callback(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue), drogon::k500InternalServerError));
    }
}

void PaperController::update(const drogon::HttpRequestPtr &request,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             long long paperId)
{
    const auto json = request->getJsonObject();
    if (!json)
    {
        callback(mathai::utils::jsonResponse(400, "invalid request body", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }

    const auto &body = *json;
    std::ostringstream sql;
    sql << "UPDATE exam_paper SET ";
    bool first = true;
    if (body.isMember("title") && body["title"].isString()) { sql << "title = " << quote(body["title"].asString()); first = false; }
    if (body.isMember("courseId") && body["courseId"].isIntegral()) { if (!first) sql << ", "; sql << "course_id = " << body["courseId"].asInt64(); first = false; }
    if (body.isMember("examYear") && (body["examYear"].isIntegral() || body["examYear"].isNull())) { if (!first) sql << ", "; sql << "exam_year = " << (body["examYear"].isNull() ? std::string("NULL") : std::to_string(body["examYear"].asInt())); first = false; }
    if (body.isMember("semester") && body["semester"].isString()) { if (!first) sql << ", "; sql << "semester = " << quote(body["semester"].asString()); first = false; }
    if (body.isMember("status") && body["status"].isIntegral()) { if (!first) sql << ", "; sql << "status = " << body["status"].asInt(); first = false; }
    if (first)
    {
        callback(mathai::utils::jsonResponse(400, "no fields to update", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }
    sql << " WHERE id = " << paperId;

    try
    {
        const auto result = mathai::utils::mysql::execute(sql.str());
        if (result.affectedRows == 0)
        {
            callback(mathai::utils::jsonResponse(404, "paper not found", Json::Value(Json::objectValue), drogon::k404NotFound));
            return;
        }
        callback(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Paper update error: " << exception.what();
        callback(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue), drogon::k500InternalServerError));
    }
}

void PaperController::disable(const drogon::HttpRequestPtr &,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                              long long paperId)
{
    try
    {
        const auto result = mathai::utils::mysql::execute("UPDATE exam_paper SET status = 0 WHERE id = " + std::to_string(paperId));
        if (result.affectedRows == 0)
        {
            callback(mathai::utils::jsonResponse(404, "paper not found", Json::Value(Json::objectValue), drogon::k404NotFound));
            return;
        }
        callback(mathai::utils::jsonResponse(200, "success", Json::Value(Json::objectValue)));
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Paper disable error: " << exception.what();
        callback(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue), drogon::k500InternalServerError));
    }
}
