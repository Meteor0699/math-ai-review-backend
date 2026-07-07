#include "utils/ErrorHandler.h"

#include <drogon/drogon.h>

#include <exception>

#include "utils/JsonResponse.h"

namespace mathai::utils
{

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
        [](drogon::HttpStatusCode statusCode) {
            if (statusCode == drogon::k404NotFound)
            {
                return error(404, "api not found", drogon::k404NotFound);
            }
            return error(static_cast<int>(statusCode), "request failed", statusCode);
        });
}

} // namespace mathai::utils
