#include <drogon/drogon.h>

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

int main()
{
    forceKeepControllers();
    mathai::utils::loadDotEnv(".env");
    drogon::app().loadConfigFile("config.json");
    mathai::utils::registerErrorHandlers();
    LOG_INFO << "math-ai-review-backend started";
    drogon::app().run();
    return 0;
}
