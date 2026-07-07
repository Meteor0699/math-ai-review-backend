#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <string>

namespace mathai::utils
{

struct Pagination
{
    int page{1};
    int pageSize{20};
};

inline Pagination parsePagination(const drogon::HttpRequestPtr &request)
{
    Pagination p;
    const auto pageStr = request->getParameter("page");
    if (!pageStr.empty())
    {
        try { p.page = std::max(1, std::stoi(pageStr)); } catch (...) {}
    }
    const auto pageSizeStr = request->getParameter("pageSize");
    if (!pageSizeStr.empty())
    {
        try { p.pageSize = std::clamp(std::stoi(pageSizeStr), 1, 100); } catch (...) {}
    }
    return p;
}

inline drogon::HttpResponsePtr jsonResponse(
    int code,
    const std::string &message,
    const Json::Value &data = Json::Value(Json::objectValue),
    drogon::HttpStatusCode status = drogon::k200OK)
{
    Json::Value body;
    body["code"] = code;
    body["message"] = message;
    body["data"] = data;

    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(status);
    return response;
}

inline drogon::HttpResponsePtr success(
    const Json::Value &data = Json::Value(Json::objectValue),
    const std::string &message = "success")
{
    return jsonResponse(0, message, data, drogon::k200OK);
}

inline drogon::HttpResponsePtr created(
    const Json::Value &data = Json::Value(Json::objectValue),
    const std::string &message = "created")
{
    return jsonResponse(0, message, data, drogon::k201Created);
}

inline drogon::HttpResponsePtr error(
    int code,
    const std::string &message,
    drogon::HttpStatusCode status = drogon::k400BadRequest)
{
    return jsonResponse(code, message, Json::Value(Json::objectValue), status);
}

} // namespace mathai::utils
