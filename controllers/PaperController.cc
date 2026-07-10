#include "controllers/PaperController.h"

#include <drogon/drogon.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_set>

#include <openssl/rand.h>

#include "clients/AiClient.h"
#include "utils/JsonResponse.h"
#include "utils/JwtUtil.h"
#include "utils/MysqlSyncClient.h"
#include "utils/RequestValidation.h"

namespace
{

std::string quote(const std::string &value) { return mathai::utils::mysql::quote(value); }

std::optional<mathai::utils::JwtClaims> claimsFromRequest(const drogon::HttpRequestPtr &request)
{
    const auto authHeader = request->getHeader("Authorization");
    const std::string prefix = "Bearer ";
    if (authHeader.rfind(prefix, 0) != 0)
    {
        return std::nullopt;
    }
    return mathai::utils::verifyJwt(authHeader.substr(prefix.size()));
}

bool isMyPapersRoute(const drogon::HttpRequestPtr &request)
{
    return request->path().rfind("/api/my/papers", 0) == 0;
}

bool isAdminPapersRoute(const drogon::HttpRequestPtr &request)
{
    return request->path().rfind("/api/admin/papers", 0) == 0;
}

Json::Value paperRowToJson(const mathai::utils::mysql::Row &row, bool includeOwnerDetails = false)
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
    item["fileUrl"] = "/api/papers/" + std::to_string(id) + "/download";
    item["fileSize"] = Json::Int64(row.getInt64("file_size"));
    item["fileType"] = row.getString("file_type");
    item["splitStatus"] = row.getString("split_status");
    item["status"] = row.getInt("status");
    item["courseName"] = row.getString("course_name");
    item["visibility"] = row.getString("visibility", "public");
    item["isPublic"] = item["visibility"].asString() == "public";
    item["aiReviewStatus"] = row.getString("ai_review_status", "not_required");
    if (includeOwnerDetails)
    {
        item["ownerUserId"] = row.isNull("owner_user_id") ? Json::Value(Json::nullValue) : Json::Value(Json::Int64(row.getInt64("owner_user_id")));
        item["aiReviewComment"] = row.getString("ai_review_comment");
    }
    return item;
}

std::string generateStoredFilename(const std::string &extension)
{
    std::array<unsigned char, 16> randomBytes{};
    if (RAND_bytes(randomBytes.data(), static_cast<int>(randomBytes.size())) != 1)
    {
        throw std::runtime_error("failed to generate secure upload filename");
    }

    std::ostringstream name;
    name << std::hex << std::setfill('0');
    for (const auto byte : randomBytes)
    {
        name << std::setw(2) << static_cast<int>(byte);
    }
    name << "." << extension;
    return name.str();
}

std::string lowerExt(std::string ext)
{
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    while (!ext.empty() && ext.front() == '.')
    {
        ext.erase(ext.begin());
    }
    return ext;
}

bool startsWith(const std::string_view content, const std::initializer_list<unsigned char> signature)
{
    if (content.size() < signature.size())
    {
        return false;
    }
    std::size_t index = 0;
    for (const auto byte : signature)
    {
        if (static_cast<unsigned char>(content[index++]) != byte)
        {
            return false;
        }
    }
    return true;
}

bool isLikelyText(const std::string_view content)
{
    if (content.empty() || content.find('\0') != std::string_view::npos)
    {
        return false;
    }

    std::size_t controlCharacters = 0;
    for (const auto ch : content)
    {
        const auto byte = static_cast<unsigned char>(ch);
        if (byte < 0x20 && ch != '\n' && ch != '\r' && ch != '\t')
        {
            ++controlCharacters;
        }
    }
    return controlCharacters * 100 <= content.size();
}

bool matchesDeclaredFileType(const drogon::HttpFile &file, const std::string &extension)
{
    const auto content = file.fileContent();
    if (extension == "pdf") return startsWith(content, {'%', 'P', 'D', 'F', '-'});
    if (extension == "doc") return startsWith(content, {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1});
    if (extension == "docx") return startsWith(content, {'P', 'K', 0x03, 0x04});
    if (extension == "txt") return isLikelyText(content);
    return false;
}

std::string safeOriginalFilename(std::string filename)
{
    for (auto &ch : filename)
    {
        if (ch == '\r' || ch == '\n' || ch == '"' || ch == ';' || ch == '/' || ch == '\\')
        {
            ch = '_';
        }
    }
    if (filename.size() > 200)
    {
        filename.resize(200);
    }
    return filename.empty() ? "paper" : filename;
}

std::string percentEncodeHeaderValue(const std::string &value)
{
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;
    for (const auto ch : value)
    {
        const auto byte = static_cast<unsigned char>(ch);
        if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
            (byte >= '0' && byte <= '9') || byte == '.' || byte == '-' || byte == '_')
        {
            encoded << ch;
        }
        else
        {
            encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
    }
    return encoded.str();
}

std::string uploadDirectory()
{
    const auto *configured = std::getenv("UPLOAD_DIR");
    if (configured != nullptr && configured[0] != '\0')
    {
        return configured;
    }
    return drogon::app().getCustomConfig().get("upload_dir", "./uploads/papers").asString();
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

std::string readTextPreview(const std::string &filePath, const std::string &fileType)
{
    const auto ext = lowerExt(fileType);
    if (ext != "txt" && ext != ".txt")
    {
        return "";
    }

    std::ifstream input(filePath, std::ios::binary);
    if (!input)
    {
        return "";
    }
    std::string content(4096, '\0');
    input.read(content.data(), static_cast<std::streamsize>(content.size()));
    content.resize(static_cast<std::size_t>(input.gcount()));
    return content;
}

std::string buildPaperReviewPrompt(const std::string &courseId,
                                   const std::string &title,
                                   const std::string &year,
                                   const std::string &originalFilename,
                                   const std::string &fileType,
                                   const std::string &textPreview)
{
    std::ostringstream prompt;
    prompt << "你是高校数学复习平台的试卷上传审核员。请判断学生上传的文件是否可以视为数学课程试卷、数学测试卷、数学练习卷或数学考试资料。\n"
           << "只允许数学相关试卷进入系统。非数学、无关资料、广告、空文件说明、生活文档、代码文件等都必须拒绝。如果证据不足，也要拒绝。\n\n"
           << "请严格按以下格式输出，第一行只能是 PASS 或 REJECT：\n"
           << "PASS\n原因：...\n"
           << "或\n"
           << "REJECT\n原因：...\n\n"
           << "课程ID：" << courseId << "\n"
           << "试卷标题：" << title << "\n"
           << "年份：" << (year.empty() ? "未填写" : year) << "\n"
           << "原始文件名：" << originalFilename << "\n"
           << "文件类型：" << fileType << "\n";
    if (!textPreview.empty())
    {
        prompt << "文本预览：\n" << textPreview << "\n";
    }
    else
    {
        prompt << "文本预览：无法读取正文，请主要依据标题、课程、文件名和类型判断；证据不足时拒绝。\n";
    }
    return prompt.str();
}

bool aiApprovedMathPaper(const std::string &content)
{
    auto normalized = content;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    const auto passPos = normalized.find("PASS");
    const auto rejectPos = normalized.find("REJECT");
    return passPos != std::string::npos && (rejectPos == std::string::npos || passPos < rejectPos);
}

} // namespace

void PaperController::list(const drogon::HttpRequestPtr &request,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    try
    {
        const auto p = mathai::utils::parsePagination(request);
        const auto offset = static_cast<int>((p.page - 1) * p.pageSize);
        const auto claims = claimsFromRequest(request);
        const auto myRoute = isMyPapersRoute(request);
        const auto adminRoute = isAdminPapersRoute(request);
        std::ostringstream whereClause;
        whereClause << " WHERE ep.status = 1";
        if (myRoute)
        {
            if (!claims)
            {
                callback(mathai::utils::jsonResponse(401, "missing token", Json::Value(Json::objectValue), drogon::k401Unauthorized));
                return;
            }
            whereClause << " AND ep.owner_user_id = " << claims->userId;
        }
        else if (!adminRoute)
        {
            whereClause << " AND ep.visibility = 'public'";
        }
        const auto courseId = request->getParameter("courseId");
        const auto parsedCourseId = mathai::utils::parseInteger(courseId, 1, INT64_MAX);
        if (!courseId.empty() && !parsedCourseId)
        {
            callback(mathai::utils::error(400, "invalid courseId", drogon::k400BadRequest));
            return;
        }
        if (parsedCourseId) whereClause << " AND ep.course_id = " << *parsedCourseId;
        const auto examYear = request->getParameter("examYear");
        const auto year = examYear.empty() ? request->getParameter("year") : examYear;
        const auto parsedYear = mathai::utils::parseInteger(year, 1900, 2155);
        if (!year.empty() && !parsedYear)
        {
            callback(mathai::utils::error(400, "invalid examYear", drogon::k400BadRequest));
            return;
        }
        if (parsedYear) whereClause << " AND ep.exam_year = " << *parsedYear;
        const auto whereStr = whereClause.str();
        const auto countResult = mathai::utils::mysql::execute("SELECT COUNT(*) AS total FROM exam_paper ep" + whereStr);
        const auto total = static_cast<Json::UInt64>(countResult.rows[0].getInt64("total"));
        const auto result = mathai::utils::mysql::execute(
            "SELECT ep.id, ep.course_id, ep.title, ep.exam_year, ep.semester, ep.original_filename, ep.stored_filename, "
            "ep.file_path, ep.file_size, ep.file_type, ep.split_status, ep.status, ep.owner_user_id, ep.visibility, "
            "ep.ai_review_status, ep.ai_review_comment, c.name AS course_name "
            "FROM exam_paper ep LEFT JOIN course c ON ep.course_id = c.id" +
            whereStr + " ORDER BY ep.exam_year DESC, ep.id DESC LIMIT " + std::to_string(p.pageSize) +
            " OFFSET " + std::to_string(offset));
        Json::Value items(Json::arrayValue);
        for (const auto &row : result.rows) items.append(paperRowToJson(row, myRoute || adminRoute));
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

void PaperController::detail(const drogon::HttpRequestPtr &request,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             long long paperId)
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT ep.id, ep.course_id, ep.title, ep.exam_year, ep.semester, ep.original_filename, ep.stored_filename, "
            "ep.file_path, ep.file_size, ep.file_type, ep.split_status, ep.status, ep.owner_user_id, ep.visibility, "
            "ep.ai_review_status, ep.ai_review_comment, c.name AS course_name "
            "FROM exam_paper ep LEFT JOIN course c ON ep.course_id = c.id WHERE ep.id = " + std::to_string(paperId));
        if (result.rows.empty())
        {
            callback(mathai::utils::jsonResponse(404, "paper not found", Json::Value(Json::objectValue), drogon::k404NotFound));
            return;
        }
        const auto &row = result.rows[0];
        const auto visibility = row.getString("visibility", "public");
        const auto claims = claimsFromRequest(request);
        if (visibility == "private")
        {
            if (!claims || (claims->role != "admin" && claims->userId != row.getInt64("owner_user_id")))
            {
                callback(mathai::utils::jsonResponse(403, "paper is private", Json::Value(Json::objectValue), drogon::k403Forbidden));
                return;
            }
        }
        const auto includeOwnerDetails = claims &&
            (claims->role == "admin" || claims->userId == row.getInt64("owner_user_id"));
        callback(mathai::utils::jsonResponse(200, "success", paperRowToJson(row, includeOwnerDetails)));
    }
    catch (const std::exception &exception)
    {
        LOG_ERROR << "Paper detail query error: " << exception.what();
        callback(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue), drogon::k500InternalServerError));
    }
}

void PaperController::download(const drogon::HttpRequestPtr &request,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               long long paperId)
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id, original_filename, file_path, file_type, status, owner_user_id, visibility FROM exam_paper WHERE id = " +
            std::to_string(paperId) + " AND status = 1");
        if (result.rows.empty())
        {
            callback(mathai::utils::jsonResponse(404, "paper not found", Json::Value(Json::objectValue), drogon::k404NotFound));
            return;
        }

        const auto &row = result.rows[0];
        if (row.getString("visibility", "public") == "private")
        {
            const auto claims = claimsFromRequest(request);
            if (!claims || (claims->role != "admin" && claims->userId != row.getInt64("owner_user_id")))
            {
                callback(mathai::utils::jsonResponse(403, "paper is private", Json::Value(Json::objectValue), drogon::k403Forbidden));
                return;
            }
        }

        const auto filePath = row.getString("file_path");
        if (filePath.empty() || !std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath))
        {
            callback(mathai::utils::jsonResponse(404, "paper file not found", Json::Value(Json::objectValue), drogon::k404NotFound));
            return;
        }

        auto response = drogon::HttpResponse::newFileResponse(filePath);
        response->setContentTypeString(fileContentType(row.getString("file_type")));
        const auto originalFilename = safeOriginalFilename(row.getString("original_filename"));
        const auto fallbackFilename = "paper." + lowerExt(row.getString("file_type"));
        response->addHeader(
            "Content-Disposition",
            "inline; filename=\"" + fallbackFilename + "\"; filename*=UTF-8''" +
                percentEncodeHeaderValue(originalFilename));
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
    const auto originalFilename = safeOriginalFilename(file.getFileName());
    const auto ext = lowerExt(std::string(file.getFileExtension()));
    static const std::unordered_set<std::string> allowedExts{"pdf", "doc", "docx", "txt"};
    if (allowedExts.find(ext) == allowedExts.end())
    {
        callback(mathai::utils::jsonResponse(400, "unsupported file type", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }
    constexpr size_t maxFileSize = 20 * 1024 * 1024;
    if (file.fileLength() == 0 || file.fileLength() > maxFileSize)
    {
        callback(mathai::utils::jsonResponse(400, "file must be between 1 byte and 20 MB", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }
    if (!matchesDeclaredFileType(file, ext))
    {
        callback(mathai::utils::jsonResponse(400, "file content does not match its extension", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }

    const auto storedFilename = generateStoredFilename(ext);
    const auto claims = claimsFromRequest(request);
    const auto studentUpload = isMyPapersRoute(request);
    if (studentUpload && !claims)
    {
        callback(mathai::utils::jsonResponse(401, "missing token", Json::Value(Json::objectValue), drogon::k401Unauthorized));
        return;
    }

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
    auto visibility = findParam("visibility");
    if (visibility != "public")
    {
        visibility = "private";
    }
    if (!studentUpload)
    {
        visibility = "public";
    }
    if (courseId.empty() || title.empty())
    {
        callback(mathai::utils::jsonResponse(400, "courseId and title are required", Json::Value(Json::objectValue), drogon::k400BadRequest));
        return;
    }
    const auto parsedCourseId = mathai::utils::parseInteger(courseId, 1, INT64_MAX);
    const auto parsedYear = mathai::utils::parseInteger(year, 1900, 2155);
    if (!parsedCourseId || (!year.empty() && !parsedYear))
    {
        callback(mathai::utils::error(400, "invalid courseId or examYear", drogon::k400BadRequest));
        return;
    }

    const auto uploadDir = uploadDirectory();
    const auto filePath = (std::filesystem::path(uploadDir) / storedFilename).string();
    std::error_code fileError;
    std::filesystem::create_directories(uploadDir, fileError);
    if (fileError || file.saveAs(filePath) != 0)
    {
        LOG_ERROR << "Failed to save uploaded paper: " << fileError.message();
        callback(mathai::utils::jsonResponse(500, "failed to store uploaded file", Json::Value(Json::objectValue), drogon::k500InternalServerError));
        return;
    }

    auto responseCallback = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(callback));
    const auto fileSize = file.fileLength();
    const auto ownerUserId = studentUpload ? claims->userId : 0;
    const auto insertPaper =
        [courseId = *parsedCourseId, year = parsedYear, title, originalFilename, storedFilename, filePath, fileSize, ext, ownerUserId, visibility,
         studentUpload, responseCallback](const std::string &reviewStatus, const std::string &reviewComment) {
            try
            {
                const auto result = mathai::utils::mysql::execute(
                    "INSERT INTO exam_paper (course_id, title, exam_year, original_filename, stored_filename, file_path, file_size, file_type, owner_user_id, visibility, ai_review_status, ai_review_comment) VALUES (" +
                    std::to_string(courseId) + ", " + quote(title) + ", " +
                    (year ? std::to_string(*year) : "NULL") + ", " + quote(originalFilename) + ", " +
                    quote(storedFilename) + ", " + quote(filePath) + ", " + std::to_string(fileSize) + ", " + quote(ext) + ", " +
                    (studentUpload ? std::to_string(ownerUserId) : std::string("NULL")) + ", " + quote(visibility) + ", " +
                    quote(reviewStatus) + ", " + quote(reviewComment.substr(0, 1000)) + ")");
                Json::Value data;
                data["id"] = Json::UInt64(result.insertId);
                data["aiReviewStatus"] = reviewStatus;
                (*responseCallback)(mathai::utils::jsonResponse(200, "success", data));
            }
            catch (const std::exception &exception)
            {
                LOG_ERROR << "Paper upload error: " << exception.what();
                std::filesystem::remove(filePath);
                (*responseCallback)(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue), drogon::k500InternalServerError));
            }
        };

    if (!studentUpload)
    {
        insertPaper("not_required", "admin upload");
        return;
    }

    const auto prompt = buildPaperReviewPrompt(courseId, title, year, originalFilename, ext, readTextPreview(filePath, ext));
    mathai::clients::AiClient aiClient;
    aiClient.generateExplanation(prompt, [filePath, responseCallback, insertPaper](mathai::clients::AiResult result) {
        if (!result.success)
        {
            std::filesystem::remove(filePath);
            Json::Value data;
            data["reason"] = result.errorMessage;
            (*responseCallback)(mathai::utils::jsonResponse(502, "paper AI review failed", data, drogon::k502BadGateway));
            return;
        }

        if (!aiApprovedMathPaper(result.content))
        {
            std::filesystem::remove(filePath);
            Json::Value data;
            data["review"] = result.content;
            (*responseCallback)(mathai::utils::jsonResponse(400, "uploaded file is not recognized as a math exam paper", data, drogon::k400BadRequest));
            return;
        }

        insertPaper("approved", result.content);
    });
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
    if (body.isMember("visibility") && body["visibility"].isString()) { if (!first) sql << ", "; sql << "visibility = " << quote(body["visibility"].asString() == "public" ? "public" : "private"); first = false; }
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
