#include "utils/ErrorHandler.h"

#include <drogon/drogon.h>

#include <exception>
#include <filesystem>
#include <string>

#include "utils/JsonResponse.h"

namespace mathai::utils
{
namespace
{

std::filesystem::path frontendDistRoot()
{
    const std::filesystem::path candidates[] = {
        "frontend_dist",
        "frontend/dist",
        "../frontend/dist",
    };

    for (const auto &candidate : candidates)
    {
        if (std::filesystem::exists(candidate / "index.html"))
        {
            return candidate;
        }
    }
    return {};
}

bool isApiPath(const std::string &path)
{
    return path == "/api" || path.rfind("/api/", 0) == 0 ||
           path == "/api/v1" || path.rfind("/api/v1/", 0) == 0;
}

bool looksLikeStaticAsset(const std::string &path)
{
    return path.find('.') != std::string::npos;
}

std::string trimLeadingSlash(const std::string &path)
{
    const auto first = path.find_first_not_of('/');
    return first == std::string::npos ? "" : path.substr(first);
}

drogon::HttpResponsePtr frontendResponse(const drogon::HttpRequestPtr &request)
{
    const auto root = frontendDistRoot();
    if (root.empty())
    {
        return nullptr;
    }

    const auto path = request->path();
    auto target = root / trimLeadingSlash(path);
    if (path == "/" || !std::filesystem::exists(target))
    {
        target = root / "index.html";
    }

    if (std::filesystem::is_directory(target))
    {
        target /= "index.html";
    }

    if (!std::filesystem::exists(target))
    {
        return nullptr;
    }

    if (looksLikeStaticAsset(path) && target.filename() == "index.html")
    {
        return nullptr;
    }

    (void)request;
    return drogon::HttpResponse::newFileResponse(target.string(), "", drogon::CT_NONE, "");
}

} // namespace

void registerErrorHandlers()
{
    drogon::app().setExceptionHandler(
        [](const std::exception &exception,
           const drogon::HttpRequestPtr &,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            LOG_ERROR << "Unhandled exception: " << exception.what();
            callback(error(500, "internal server error", drogon::k500InternalServerError));
        });

    drogon::app().setCustomErrorHandler(
        [](drogon::HttpStatusCode statusCode, const drogon::HttpRequestPtr &request) {
            if (statusCode == drogon::k404NotFound && !isApiPath(request->path()))
            {
                const auto response = frontendResponse(request);
                if (response)
                {
                    return response;
                }
            }

            if (statusCode == drogon::k404NotFound)
            {
                return error(404, "api not found", drogon::k404NotFound);
            }

            return error(static_cast<int>(statusCode), "request failed", statusCode);
        });
}

} // namespace mathai::utils
