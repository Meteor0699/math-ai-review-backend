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

int main()
{
    mathai::utils::loadDotEnv(".env");
    drogon::app().loadConfigFile("config.json");

    // Explicitly register controllers to prevent the linker from discarding
    // them in Release builds, which would cause all API routes to return 404.
    drogon::DrClassMap::registerClass<AuthController>();
    drogon::DrClassMap::registerClass<CourseController>();
    drogon::DrClassMap::registerClass<ChapterController>();
    drogon::DrClassMap::registerClass<KnowledgeController>();
    drogon::DrClassMap::registerClass<QuestionController>();
    drogon::DrClassMap::registerClass<PaperController>();
    drogon::DrClassMap::registerClass<AiController>();
    drogon::DrClassMap::registerClass<UserController>();

    mathai::utils::registerErrorHandlers();
    LOG_INFO << "math-ai-review-backend started";
    drogon::app().run();
    return 0;
}
