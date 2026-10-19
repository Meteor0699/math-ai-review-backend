#include <drogon/drogon.h>

#include <filesystem>
#include <optional>
#include <string>

#include "controllers/AiController.h"
#include "controllers/AdminStatsController.h"
#include "controllers/AgentController.h"
#include "controllers/AuthController.h"
#include "controllers/ChapterController.h"
#include "controllers/CourseController.h"
#include "controllers/HealthController.h"
#include "controllers/KnowledgeController.h"
#include "controllers/PaperController.h"
#include "controllers/QuestionController.h"
#include "controllers/StudyController.h"
#include "controllers/UserController.h"
#include "filters/RateLimitFilter.h"
#include "utils/EnvLoader.h"
#include "utils/ErrorHandler.h"
#include "utils/JsonResponse.h"
#include "utils/JwtUtil.h"

// Force the linker to keep every controller translation unit so that their
// static initializers (which register API routes) are not discarded in
// Release builds.  Without these references all routes would return 404.
static void forceKeepControllers()
{
    volatile const bool *refs[] = {
        &AuthController::isAutoCreation,
        &AdminStatsController::isAutoCreation,
        &CourseController::isAutoCreation,
        &ChapterController::isAutoCreation,
        &KnowledgeController::isAutoCreation,
        &QuestionController::isAutoCreation,
        &PaperController::isAutoCreation,
        &AiController::isAutoCreation,
        &AgentController::isAutoCreation,
        &StudyController::isAutoCreation,
        &UserController::isAutoCreation,
        &HealthController::isAutoCreation,
        &RateLimitFilter::isAutoCreation,
    };
    (void)refs;
}

// The document root does not move while the process is running, so it is
// resolved once instead of issuing three filesystem probes per static request.
static const std::filesystem::path &frontendDist()
{
    static const std::filesystem::path root = [] {
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
        return std::filesystem::path{};
    }();
    return root;
}

static std::string trimLeadingSlash(const std::string &path)
{
    const auto first = path.find_first_not_of('/');
    return first == std::string::npos ? "" : path.substr(first);
}

static bool isStaticAssetPath(const std::string &path)
{
    return path.rfind("/assets/", 0) == 0 || path.rfind("/textbook-covers/", 0) == 0;
}

static std::optional<std::filesystem::path> resolveFrontendPath(
    const std::filesystem::path &root,
    const std::string &requestPath)
{
    const auto relativeText = trimLeadingSlash(requestPath);
    if (relativeText.find('\\') != std::string::npos)
    {
        return std::nullopt;
    }

    const std::filesystem::path relative(relativeText);
    if (relative.is_absolute() || relative.has_root_path())
    {
        return std::nullopt;
    }
    for (const auto &component : relative)
    {
        if (component == "..")
        {
            return std::nullopt;
        }
    }

    std::error_code error;
    const auto canonicalRoot = std::filesystem::weakly_canonical(root, error);
    if (error)
    {
        return std::nullopt;
    }
    const auto target = std::filesystem::weakly_canonical(canonicalRoot / relative, error);
    if (error)
    {
        return std::nullopt;
    }

    const auto pathWithinRoot = target.lexically_relative(canonicalRoot);
    if (pathWithinRoot.empty() || pathWithinRoot.is_absolute())
    {
        return std::nullopt;
    }
    for (const auto &component : pathWithinRoot)
    {
        if (component == "..")
        {
            return std::nullopt;
        }
    }
    return target;
}

static drogon::HttpResponsePtr frontendFileResponse(const drogon::HttpRequestPtr &request)
{
    const auto &root = frontendDist();
    if (root.empty())
    {
        return mathai::utils::error(404, "api not found", drogon::k404NotFound);
    }

    const auto path = request->path();
    auto resolved = resolveFrontendPath(root, path);
    if (!resolved)
    {
        return mathai::utils::error(404, "resource not found", drogon::k404NotFound);
    }

    auto target = *resolved;
    auto servesShell = false;
    if (path == "/" || !std::filesystem::exists(target))
    {
        if (isStaticAssetPath(path))
        {
            return mathai::utils::error(404, "resource not found", drogon::k404NotFound);
        }
        target = root / "index.html";
        servesShell = true;
    }

    if (std::filesystem::is_directory(target))
    {
        target /= "index.html";
        servesShell = true;
    }

    if (!std::filesystem::exists(target))
    {
        return mathai::utils::error(404, "api not found", drogon::k404NotFound);
    }

    auto response = drogon::HttpResponse::newFileResponse(target.string(), "", drogon::CT_NONE, "");
    if (servesShell)
    {
        // The HTML shell points at content hashed bundles, so it must be
        // revalidated on every load; caching it would keep serving the previous
        // build after a deploy.
        response->addHeader("Cache-Control", "no-cache");
    }
    else if (path.rfind("/assets/", 0) == 0)
    {
        // Vite emits content hashed file names, so these can never go stale
        // under the same URL.
        response->addHeader("Cache-Control", "public, max-age=31536000, immutable");
    }
    else
    {
        response->addHeader("Cache-Control", "public, max-age=86400");
    }
    return response;
}

static void registerFrontendRoot()
{
    drogon::app().registerHandler(
        "/",
        [](const drogon::HttpRequestPtr &request,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            callback(frontendFileResponse(request));
        },
        {drogon::Get});

    drogon::app().registerHandlerViaRegex(
        "^/(login|home|courses.*|chapters.*|knowledge.*|questions.*|study.*|papers.*|admin.*|assets/.*|textbook-covers/.*|robots\\.txt|favicon\\.ico|favicon\\.svg)$",
        [](const drogon::HttpRequestPtr &request,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            callback(frontendFileResponse(request));
        },
        {drogon::Get});
}

int main()
{
    forceKeepControllers();
    mathai::utils::loadDotEnv(".env");
    drogon::app().loadConfigFile("config.json");
    // Text assets (JS/CSS/JSON/HTML) are compressed on the fly: the built SPA
    // entry is ~1.2 MB of JavaScript and Drogon sends it uncompressed by default.
    drogon::app().enableGzip(true).enableBrotli(true);
    mathai::utils::validateJwtConfiguration();
    mathai::utils::registerErrorHandlers();
    registerFrontendRoot();
    LOG_INFO << "math-ai-review-backend started";
    drogon::app().run();
    return 0;
}
