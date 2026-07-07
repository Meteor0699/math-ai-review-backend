#include <drogon/drogon.h>

#include <filesystem>

#include "controllers/AiController.h"
#include "controllers/AuthController.h"
#include "controllers/ChapterController.h"
#include "controllers/CourseController.h"
#include "controllers/KnowledgeController.h"
#include "controllers/PaperController.h"
#include "controllers/QuestionController.h"
#include "controllers/UserController.h"
#include "utils/EnvLoader.h"
#include "utils/ErrorHandler.h"
#include "utils/JsonResponse.h"

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
        &UserController::isAutoCreation,
    };
    (void)refs;
}

static std::filesystem::path findFrontendIndex()
{
    const std::filesystem::path candidates[] = {
        "frontend_dist/index.html",
        "frontend/dist/index.html",
        "../frontend/dist/index.html",
    };

    for (const auto &candidate : candidates)
    {
        if (std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }
    return {};
}

static void registerFrontendRoot()
{
    drogon::app().registerHandler(
        "/",
        [](const drogon::HttpRequestPtr &request,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const auto index = findFrontendIndex();
            if (!index.empty())
            {
                (void)request;
                callback(drogon::HttpResponse::newFileResponse(index.string(), "", drogon::CT_NONE, ""));
                return;
            }
            callback(mathai::utils::error(404, "api not found", drogon::k404NotFound));
        },
        {drogon::Get});
}

int main()
{
    forceKeepControllers();
    mathai::utils::loadDotEnv(".env");
    drogon::app().loadConfigFile("config.json");
    mathai::utils::registerErrorHandlers();
    registerFrontendRoot();
    LOG_INFO << "math-ai-review-backend started";
    drogon::app().run();
    return 0;
}
