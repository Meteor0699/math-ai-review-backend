#include "utils/ErrorHandler.h"

#include <drogon/drogon.h>

#include <exception>

#include "utils/JsonResponse.h"

namespace mathai::utils
{

void registerErrorHandlers()
{
    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr &, const drogon::HttpResponsePtr &response) {
            response->addHeader("Content-Security-Policy",
                                "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; "
                                "font-src 'self' data:; img-src 'self' data: https:; connect-src 'self' https:; "
                                "object-src 'none'; base-uri 'self'; frame-ancestors 'none'");
            response->addHeader("X-Content-Type-Options", "nosniff");
            response->addHeader("X-Frame-Options", "DENY");
            response->addHeader("Referrer-Policy", "strict-origin-when-cross-origin");
            response->addHeader("Permissions-Policy", "camera=(), microphone=(), geolocation=()");
        });

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
