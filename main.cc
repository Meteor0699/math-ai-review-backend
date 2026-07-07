#include <drogon/drogon.h>

#include "utils/EnvLoader.h"
#include "utils/ErrorHandler.h"

int main()
{
    mathai::utils::loadDotEnv(".env");
    drogon::app().loadConfigFile("config.json");
    mathai::utils::registerErrorHandlers();
    LOG_INFO << "math-ai-review-backend started";
    drogon::app().run();
    return 0;
}
