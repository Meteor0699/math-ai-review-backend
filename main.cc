#include <drogon/drogon.h>

#include <filesystem>
#include <string>

#include "controllers/AiController.h"
#include "controllers/AuthController.h"
#include "controllers/ChapterController.h"
#include "controllers/CourseController.h"
#include "controllers/HealthController.h"
#include "controllers/KnowledgeController.h"
#include "controllers/PaperController.h"
#include "controllers/QuestionController.h"
#include "controllers/StudyController.h"
#include "controllers/UserController.h"
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
        &CourseController::isAutoCreation,
        &ChapterController::isAutoCreation,
        &KnowledgeController::isAutoCreation,
        &QuestionController::isAutoCreation,
        &PaperController::isAutoCreation,
        &AiController::isAutoCreation,
        &StudyController::isAutoCreation,
        &UserController::isAutoCreation,
        &HealthController::isAutoCreation,
    };
    (void)refs;
}

static std::filesystem::path findFrontendDist()
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

static std::string trimLeadingSlash(const std::string &path)
{
    const auto first = path.find_first_not_of('/');
    return first == std::string::npos ? "" : path.substr(first);
}

static drogon::HttpResponsePtr frontendFileResponse(const drogon::HttpRequestPtr &request)
{
    const auto root = findFrontendDist();
    if (root.empty())
    {
        return mathai::utils::error(404, "api not found", drogon::k404NotFound);
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
        return mathai::utils::error(404, "api not found", drogon::k404NotFound);
    }

    return drogon::HttpResponse::newFileResponse(target.string(), "", drogon::CT_NONE, "");
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
        "^/(login|home|courses.*|chapters.*|questions.*|study.*|papers.*|admin.*|assets/.*)$",
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
    mathai::utils::validateJwtConfiguration();
    mathai::utils::registerErrorHandlers();
    registerFrontendRoot();
    LOG_INFO << "math-ai-review-backend started";
    drogon::app().run();
    return 0;
}
