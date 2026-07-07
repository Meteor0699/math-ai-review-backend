#include "clients/AiClient.h"

#include <curl/curl.h>
#include <drogon/drogon.h>
#include <json/json.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace mathai::clients
{
namespace
{

struct ParsedUrl
{
    std::string origin;
    std::string path;
};

std::string getenvString(const char *name)
{
    const auto *value = std::getenv(name);
    return value == nullptr ? "" : std::string(value);
}

std::optional<ParsedUrl> parseUrl(const std::string &url)
{
    const auto schemePos = url.find("://");
    if (schemePos == std::string::npos)
    {
        return std::nullopt;
    }

    const auto pathPos = url.find('/', schemePos + 3);
    ParsedUrl parsed;
    if (pathPos == std::string::npos)
    {
        parsed.origin = url;
        parsed.path = "/";
    }
    else
    {
        parsed.origin = url.substr(0, pathPos);
        parsed.path = url.substr(pathPos);
    }
    return parsed;
}

std::string jsonToString(const Json::Value &value)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

std::string extractContent(const Json::Value &body)
{
    if (body.isMember("choices") && body["choices"].isArray() && !body["choices"].empty())
    {
        const auto &message = body["choices"][0]["message"];
        if (message.isMember("content") && message["content"].isString())
        {
            return message["content"].asString();
        }
    }

    if (body.isMember("output") && body["output"].isMember("text") && body["output"]["text"].isString())
    {
        return body["output"]["text"].asString();
    }

    return "";
}

void ensureCurlInitialized()
{
    static const int initialized = [] {
        return curl_global_init(CURL_GLOBAL_DEFAULT);
    }();
    (void)initialized;
}

size_t writeResponseBody(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    const auto bytes = size * nmemb;
    auto *body = static_cast<std::string *>(userdata);
    body->append(ptr, bytes);
    return bytes;
}

std::optional<Json::Value> parseJson(const std::string &body)
{
    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errors;
    const auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
    if (!reader->parse(body.data(), body.data() + body.size(), &root, &errors))
    {
        LOG_ERROR << "AiClient: failed to parse JSON response: " << errors;
        return std::nullopt;
    }
    return root;
}

std::string findCaBundle()
{
    const auto aiCaBundle = getenvString("AI_CA_BUNDLE");
    if (!aiCaBundle.empty())
    {
        return aiCaBundle;
    }

    const auto curlCaBundle = getenvString("CURL_CA_BUNDLE");
    if (!curlCaBundle.empty())
    {
        return curlCaBundle;
    }

    const char *candidates[] = {
        "D:/msys64/ucrt64/etc/ssl/cert.pem",
        "D:/msys64/usr/ssl/cert.pem"};
    for (const auto *candidate : candidates)
    {
        if (std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }
    return "";
}

} // namespace

void AiClient::generateExplanation(const std::string &prompt, Callback callback) const
{
    const auto apiBaseUrl = getenvString("AI_API_BASE_URL");
    const auto apiKey = getenvString("AI_API_KEY");
    const auto model = getenvString("AI_MODEL").empty() ? "deepseek-v4-flash" : getenvString("AI_MODEL");

    LOG_INFO << "AiClient: using model=" << model << ", base_url=" << apiBaseUrl;

    if (apiBaseUrl.empty())
    {
        LOG_ERROR << "AiClient: AI_API_BASE_URL is not set";
        callback({false, "", "AI_API_BASE_URL environment variable is not set", model});
        return;
    }

    if (apiKey.empty())
    {
        LOG_ERROR << "AiClient: AI_API_KEY is not set";
        callback({false, "", "AI_API_KEY environment variable is not set", model});
        return;
    }

    const auto parsedUrl = parseUrl(apiBaseUrl);
    if (!parsedUrl)
    {
        LOG_ERROR << "AiClient: cannot parse URL: " << apiBaseUrl;
        callback({false, "", "AI_API_BASE_URL is not a valid http(s) url", model});
        return;
    }

    LOG_INFO << "AiClient: sending request to " << parsedUrl->origin << parsedUrl->path;

    Json::Value requestBody;
    requestBody["model"] = model;
    requestBody["temperature"] = 0.2;
    requestBody["messages"] = Json::Value(Json::arrayValue);
    requestBody["messages"].append(Json::Value(Json::objectValue));
    requestBody["messages"][0]["role"] = "user";
    requestBody["messages"][0]["content"] = prompt;

    const auto body = jsonToString(requestBody);
    const auto caBundle = findCaBundle();
    std::thread([apiBaseUrl, apiKey, model, body, caBundle, callback = std::move(callback)]() mutable {
        ensureCurlInitialized();

        std::string responseBody;
        long httpStatus = 0;

        CURL *rawCurl = curl_easy_init();
        if (rawCurl == nullptr)
        {
            callback({false, "", "failed to initialize HTTP client", model});
            return;
        }

        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(rawCurl, curl_easy_cleanup);
        struct curl_slist *rawHeaders = nullptr;
        rawHeaders = curl_slist_append(rawHeaders, "Content-Type: application/json");
        const auto authHeader = "Authorization: Bearer " + apiKey;
        rawHeaders = curl_slist_append(rawHeaders, authHeader.c_str());
        std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(rawHeaders, curl_slist_free_all);

        curl_easy_setopt(curl.get(), CURLOPT_URL, apiBaseUrl.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
        curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeResponseBody);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBody);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 90L);
        curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "math-ai-review-backend/1.0");
        if (!caBundle.empty())
        {
            curl_easy_setopt(curl.get(), CURLOPT_CAINFO, caBundle.c_str());
        }

        const auto code = curl_easy_perform(curl.get());
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &httpStatus);

        if (code != CURLE_OK)
        {
            const auto message = std::string("AI request failed: ") + curl_easy_strerror(code);
            LOG_ERROR << "AiClient: " << message;
            callback({false, "", message, model});
            return;
        }

        if (httpStatus < 200 || httpStatus >= 300)
        {
            LOG_ERROR << "AiClient: HTTP " << httpStatus << " from AI provider, body: " << responseBody;
            callback({false, "", "AI request failed with HTTP status " + std::to_string(httpStatus) + ": " + responseBody, model});
            return;
        }

        const auto json = parseJson(responseBody);
        if (!json)
        {
            callback({false, "", "AI response is not valid JSON", model});
            return;
        }

        const auto content = extractContent(*json);
        if (content.empty())
        {
            LOG_ERROR << "AiClient: response does not contain explanation content";
            callback({false, "", "AI response does not contain explanation content", model});
            return;
        }

        LOG_INFO << "AiClient: got explanation, length=" << content.size();
        callback({true, content, "", model});
    }).detach();
}

} // namespace mathai::clients
