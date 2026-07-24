#include "clients/AiClient.h"

#include <curl/curl.h>
#include <drogon/drogon.h>
#include <json/json.h>

#include <algorithm>
#include <cstdlib>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

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

std::size_t getenvSize(const char *name, std::size_t fallback, std::size_t minimum, std::size_t maximum)
{
    const auto value = getenvString(name);
    if (value.empty()) return fallback;
    try
    {
        return std::clamp<std::size_t>(std::stoull(value), minimum, maximum);
    }
    catch (const std::exception &)
    {
        return fallback;
    }
}

std::string resolveModelName()
{
    const auto configuredModel = getenvString("AI_MODEL");
    if (configuredModel.empty() || configuredModel == "deepseek-chat")
    {
        return "deepseek-v4-flash";
    }
    return configuredModel;
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

bool ensureCurlInitialized()
{
    static const int initialized = [] {
        return curl_global_init(CURL_GLOBAL_DEFAULT);
    }();
    return initialized == CURLE_OK;
}

size_t writeResponseBody(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    const auto bytes = size * nmemb;
    auto *body = static_cast<std::string *>(userdata);
    constexpr std::size_t maximumResponseSize = 2 * 1024 * 1024;
    if (bytes > maximumResponseSize || body->size() > maximumResponseSize - bytes)
    {
        return 0;
    }
    body->append(ptr, bytes);
    return bytes;
}

class AiExecutor
{
  public:
    AiExecutor()
        : maximumQueueSize_(getenvSize("AI_QUEUE_SIZE", 32, 1, 1000))
    {
        const auto workerCount = getenvSize("AI_MAX_CONCURRENCY", 4, 1, 32);
        workers_.reserve(workerCount);
        for (std::size_t index = 0; index < workerCount; ++index)
        {
            workers_.emplace_back([this] { run(); });
        }
    }

    ~AiExecutor()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        available_.notify_all();
        for (auto &worker : workers_)
        {
            if (worker.joinable()) worker.join();
        }
    }

    bool submit(std::function<void()> task)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_ || tasks_.size() >= maximumQueueSize_)
        {
            return false;
        }
        tasks_.push_back(std::move(task));
        available_.notify_one();
        return true;
    }

  private:
    void run()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                available_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
                if (stopping_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop_front();
            }
            try
            {
                task();
            }
            catch (const std::exception &exception)
            {
                LOG_ERROR << "AiExecutor task failed: " << exception.what();
            }
            catch (...)
            {
                LOG_ERROR << "AiExecutor task failed with an unknown error";
            }
        }
    }

    std::mutex mutex_;
    std::condition_variable available_;
    std::deque<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    std::size_t maximumQueueSize_;
    bool stopping_{false};
};

AiExecutor &aiExecutor()
{
    static AiExecutor executor;
    return executor;
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

std::string AiClient::modelName() const
{
    return resolveModelName();
}

void AiClient::generateExplanation(const std::string &prompt, Callback callback) const
{
    const auto apiBaseUrl = getenvString("AI_API_BASE_URL");
    const auto apiKey = getenvString("AI_API_KEY");
    const auto model = resolveModelName();

    LOG_INFO << "AiClient: using model=" << model << ", base_url=" << apiBaseUrl;

    if (apiBaseUrl.empty())
    {
        LOG_ERROR << "AiClient: AI_API_BASE_URL is not set";
        callback({false, "", "AI_API_BASE_URL environment variable is not set", model, false});
        return;
    }

    if (apiKey.empty())
    {
        LOG_ERROR << "AiClient: AI_API_KEY is not set";
        callback({false, "", "AI_API_KEY environment variable is not set", model, false});
        return;
    }

    const auto parsedUrl = parseUrl(apiBaseUrl);
    if (!parsedUrl)
    {
        LOG_ERROR << "AiClient: cannot parse URL: " << apiBaseUrl;
        callback({false, "", "AI_API_BASE_URL is not a valid http(s) url", model, false});
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
    auto sharedCallback = std::make_shared<Callback>(std::move(callback));
    const auto accepted = aiExecutor().submit([apiBaseUrl, apiKey, model, body, caBundle, sharedCallback]() mutable {
        if (!ensureCurlInitialized())
        {
            (*sharedCallback)({false, "", "failed to initialize HTTP runtime", model, false});
            return;
        }

        std::string responseBody;
        long httpStatus = 0;

        CURL *rawCurl = curl_easy_init();
        if (rawCurl == nullptr)
        {
            (*sharedCallback)({false, "", "failed to initialize HTTP client", model, false});
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
            (*sharedCallback)({false, "", message, model, false});
            return;
        }

        if (httpStatus < 200 || httpStatus >= 300)
        {
            LOG_ERROR << "AiClient: HTTP " << httpStatus << " from AI provider";
            (*sharedCallback)({false, "", "AI provider returned HTTP status " + std::to_string(httpStatus), model, false});
            return;
        }

        const auto json = parseJson(responseBody);
        if (!json)
        {
            (*sharedCallback)({false, "", "AI response is not valid JSON", model, false});
            return;
        }

        const auto content = extractContent(*json);
        if (content.empty())
        {
            LOG_ERROR << "AiClient: response does not contain explanation content";
            (*sharedCallback)({false, "", "AI response does not contain explanation content", model, false});
            return;
        }

        LOG_INFO << "AiClient: got explanation, length=" << content.size();
        (*sharedCallback)({true, content, "", model, false});
    });
    if (!accepted)
    {
        (*sharedCallback)({false, "", "AI request queue is full", model, true});
    }
}

} // namespace mathai::clients
