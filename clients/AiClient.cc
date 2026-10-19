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
#include <map>
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

// A single streamed answer plus its tool calls must still fit the same 2MB
// budget the non-streaming path enforces.
constexpr std::size_t maximumResponseSize = 2 * 1024 * 1024;

size_t writeResponseBody(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    const auto bytes = size * nmemb;
    auto *body = static_cast<std::string *>(userdata);
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

    // Read-only probe used to answer `code: busy` before a turn is persisted.
    // A false positive is harmless: submit() re-checks under the same lock.
    bool hasCapacity() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !stopping_ && tasks_.size() < maximumQueueSize_;
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

    // hasCapacity() is const but still needs to count the queue.
    mutable std::mutex mutex_;
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

// Incremental SSE reader for the OpenAI-compatible /chat/completions stream.
// curl hands us arbitrary byte ranges, so frames are completed across calls.
class SseStreamReader
{
  public:
    SseStreamReader(AiStreamCallbacks callbacks, std::string model)
        : callbacks_(std::move(callbacks)), model_(std::move(model))
    {
        result_.modelName = model_;
    }

    // Returns false to abort the transfer (client gone or the 2MB budget is
    // exhausted).
    bool consume(char *data, std::size_t length)
    {
        if (length > maximumResponseSize || rawBytes_ > maximumResponseSize - length)
        {
            overflow_ = true;
            return false;
        }
        rawBytes_ += length;
        buffer_.append(data, length);
        if (prefix_.size() < 1024)
        {
            prefix_.append(data, std::min<std::size_t>(length, 1024 - prefix_.size()));
        }

        std::size_t newline = 0;
        while ((newline = buffer_.find('\n')) != std::string::npos)
        {
            auto line = buffer_.substr(0, newline);
            buffer_.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            if (!consumeLine(line))
            {
                return false;
            }
        }
        return true;
    }

    // Flushes a trailing frame that arrived without its final newline.
    void finish()
    {
        if (!buffer_.empty())
        {
            consumeLine(buffer_);
            buffer_.clear();
        }
    }

    AiStreamResult &result() { return result_; }
    const std::string &failureMessage() const { return failureMessage_; }
    // The raw prefix of the body, used to surface a provider error that is not
    // an SSE frame at all (for example a JSON 4xx payload).
    std::string bodyPrefix(std::size_t maximum) const
    {
        if (prefix_.size() <= maximum)
        {
            return prefix_;
        }
        std::size_t end = maximum;
        while (end > 0 && (static_cast<unsigned char>(prefix_[end - 1]) & 0xC0U) == 0x80U)
        {
            --end;
        }
        return prefix_.substr(0, end);
    }
    bool overflowed() const { return overflow_; }
    bool sawDone() const { return sawDone_; }

    // Reassembles the accumulated tool calls in provider order. The provider may
    // omit ids (some gateways only send them on the first delta), so a stable
    // synthetic id keeps the assistant/tool message pairing valid.
    std::vector<AiToolCall> toolCalls() const
    {
        std::vector<AiToolCall> calls;
        calls.reserve(partialToolCalls_.size());
        for (const auto &entry : partialToolCalls_)
        {
            if (entry.second.name.empty())
            {
                continue;
            }
            AiToolCall call;
            call.id = entry.second.id.empty() ? "call_" + std::to_string(entry.first) : entry.second.id;
            call.name = entry.second.name;
            call.arguments = entry.second.arguments;
            calls.push_back(std::move(call));
        }
        return calls;
    }

  private:
    bool cancelled() const
    {
        return callbacks_.cancelled && callbacks_.cancelled();
    }

    bool consumeLine(const std::string &line)
    {
        if (line.empty() || line.rfind(':', 0) == 0)
        {
            return true;
        }

        constexpr const char *dataPrefix = "data:";
        if (line.rfind(dataPrefix, 0) != 0)
        {
            // event:/id:/retry: fields are not needed for this provider.
            return !cancelled();
        }

        auto payload = line.substr(5);
        const auto first = payload.find_first_not_of(" \t");
        if (first == std::string::npos)
        {
            return !cancelled();
        }
        payload = payload.substr(first);
        if (payload == "[DONE]")
        {
            sawDone_ = true;
            return !cancelled();
        }
        if (payload.rfind("keep-alive", 0) == 0 || payload.rfind("ping", 0) == 0)
        {
            return !cancelled();
        }

        parseChunk(payload);
        return !cancelled();
    }

    void parseChunk(const std::string &payload)
    {
        Json::CharReaderBuilder builder;
        Json::Value chunk;
        std::string errors;
        const auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
        if (!reader->parse(payload.data(), payload.data() + payload.size(), &chunk, &errors))
        {
            LOG_DEBUG << "AiClient: skipped unparsable SSE chunk: " << errors;
            return;
        }

        if (chunk.isMember("usage") && chunk["usage"].isObject())
        {
            const auto &usage = chunk["usage"];
            if (usage.isMember("prompt_tokens")) result_.promptTokens = usage["prompt_tokens"].asInt();
            if (usage.isMember("completion_tokens")) result_.completionTokens = usage["completion_tokens"].asInt();
        }

        if (chunk.isMember("error") && chunk["error"].isObject() && failureMessage_.empty())
        {
            failureMessage_ = chunk["error"].get("message", "AI provider reported a stream error").asString();
        }

        if (!chunk.isMember("choices") || !chunk["choices"].isArray() || chunk["choices"].empty())
        {
            return;
        }

        const auto &choice = chunk["choices"][0];
        if (choice.isMember("finish_reason") && choice["finish_reason"].isString())
        {
            result_.finishReason = choice["finish_reason"].asString();
        }

        const auto &delta = choice["delta"];
        if (!delta.isObject())
        {
            return;
        }

        if (delta.isMember("content") && delta["content"].isString())
        {
            const auto text = delta["content"].asString();
            if (!text.empty())
            {
                result_.content += text;
                if (callbacks_.onDelta)
                {
                    callbacks_.onDelta(text);
                }
            }
        }

        // Thinking models stream their chain of thought separately. It is never
        // shown to the student, but it has to be handed back on the assistant
        // message that carries tool calls.
        if (delta.isMember("reasoning_content") && delta["reasoning_content"].isString())
        {
            result_.reasoningContent += delta["reasoning_content"].asString();
        }

        if (delta.isMember("tool_calls") && delta["tool_calls"].isArray())
        {
            for (const auto &piece : delta["tool_calls"])
            {
                accumulateToolCall(piece);
            }
        }
    }

    void accumulateToolCall(const Json::Value &piece)
    {
        int index = 0;
        if (piece.isMember("index") && piece["index"].isInt())
        {
            index = piece["index"].asInt();
        }
        else if (piece.isMember("index") && piece["index"].isString())
        {
            index = std::atoi(piece["index"].asString().c_str());
        }
        if (index < 0 || index > 32)
        {
            LOG_WARN << "AiClient: ignoring tool_call with out-of-range index " << index;
            return;
        }

        auto &partial = partialToolCalls_[index];
        if (piece.isMember("id") && piece["id"].isString() && !piece["id"].asString().empty())
        {
            partial.id = piece["id"].asString();
        }
        const auto &function = piece["function"];
        if (function.isObject())
        {
            if (function.isMember("name") && function["name"].isString())
            {
                partial.name += function["name"].asString();
            }
            if (function.isMember("arguments") && function["arguments"].isString())
            {
                partial.arguments += function["arguments"].asString();
            }
        }
    }

  private:
    struct PartialToolCall
    {
        std::string id;
        std::string name;
        std::string arguments;
    };

    AiStreamCallbacks callbacks_;
    std::string model_;
    std::string buffer_;
    std::string prefix_;
    std::string failureMessage_;
    std::map<int, PartialToolCall> partialToolCalls_;
    AiStreamResult result_;
    std::size_t rawBytes_{0};
    bool overflow_{false};
    bool sawDone_{false};
};

size_t writeStreamChunk(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    const auto bytes = size * nmemb;
    auto *reader = static_cast<SseStreamReader *>(userdata);
    if (!reader->consume(ptr, bytes))
    {
        return 0;
    }
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

bool AiClient::hasCapacity()
{
    return aiExecutor().hasCapacity();
}

void AiClient::generateStream(const Json::Value &messages,
                              const Json::Value &tools,
                              AiStreamCallbacks callbacks) const
{
    const auto apiBaseUrl = getenvString("AI_API_BASE_URL");
    const auto apiKey = getenvString("AI_API_KEY");
    const auto model = resolveModelName();

    const auto fail = [&callbacks, &model](const std::string &message, bool busy) {
        AiStreamResult result;
        result.success = false;
        result.errorMessage = message;
        result.modelName = model;
        result.busy = busy;
        if (callbacks.onError)
        {
            callbacks.onError(result);
        }
    };

    if (apiBaseUrl.empty())
    {
        LOG_ERROR << "AiClient: AI_API_BASE_URL is not set";
        fail("AI_API_BASE_URL environment variable is not set", false);
        return;
    }

    if (apiKey.empty())
    {
        LOG_ERROR << "AiClient: AI_API_KEY is not set";
        fail("AI_API_KEY environment variable is not set", false);
        return;
    }

    if (!parseUrl(apiBaseUrl))
    {
        LOG_ERROR << "AiClient: cannot parse URL: " << apiBaseUrl;
        fail("AI_API_BASE_URL is not a valid http(s) url", false);
        return;
    }

    Json::Value requestBody;
    requestBody["model"] = model;
    requestBody["temperature"] = 0.2;
    requestBody["stream"] = true;
    requestBody["messages"] = messages;
    if (tools.isArray() && !tools.empty())
    {
        requestBody["tools"] = tools;
        requestBody["tool_choice"] = "auto";
    }

    const auto body = jsonToString(requestBody);
    const auto caBundle = findCaBundle();
    auto sharedCallbacks = std::make_shared<AiStreamCallbacks>(std::move(callbacks));

    const auto accepted = aiExecutor().submit(
        [apiBaseUrl, apiKey, model, body, caBundle, sharedCallbacks]() mutable {
            if (!ensureCurlInitialized())
            {
                AiStreamResult result;
                result.errorMessage = "failed to initialize HTTP runtime";
                result.modelName = model;
                if (sharedCallbacks->onError) sharedCallbacks->onError(result);
                return;
            }

            SseStreamReader reader(*sharedCallbacks, model);
            long httpStatus = 0;
            bool transportFailed = false;
            std::string transportMessage;
            std::string failureDetail;

            CURL *rawCurl = curl_easy_init();
            if (rawCurl == nullptr)
            {
                AiStreamResult result;
                result.errorMessage = "failed to initialize HTTP client";
                result.modelName = model;
                if (sharedCallbacks->onError) sharedCallbacks->onError(result);
                return;
            }

            std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(rawCurl, curl_easy_cleanup);
            struct curl_slist *rawHeaders = nullptr;
            rawHeaders = curl_slist_append(rawHeaders, "Content-Type: application/json");
            rawHeaders = curl_slist_append(rawHeaders, "Accept: text/event-stream");
            const auto authHeader = "Authorization: Bearer " + apiKey;
            rawHeaders = curl_slist_append(rawHeaders, authHeader.c_str());
            std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(rawHeaders, curl_slist_free_all);

            curl_easy_setopt(curl.get(), CURLOPT_URL, apiBaseUrl.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
            curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
            curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeStreamChunk);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &reader);
            // Streamed answers are read incrementally, so the same connect and
            // overall budgets as generateExplanation apply.
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
            reader.finish();

            auto &result = reader.result();
            result.modelName = model;

            const auto cancelled = sharedCallbacks->cancelled && sharedCallbacks->cancelled();
            if (cancelled)
            {
                // The client went away: the partial text is still returned so
                // the caller can persist what was produced.
                result.success = true;
                result.finishReason = result.finishReason.empty() ? "cancelled" : result.finishReason;
                if (sharedCallbacks->onFinished) sharedCallbacks->onFinished(result);
                return;
            }

            if (code != CURLE_OK)
            {
                transportFailed = true;
                transportMessage = std::string("AI request failed: ") + curl_easy_strerror(code);
            }
            else if (httpStatus < 200 || httpStatus >= 300)
            {
                transportFailed = true;
                transportMessage = "AI provider returned HTTP status " + std::to_string(httpStatus);
                // The provider's own explanation is the only way to tell a bad
                // request apart from a bad credential, so keep it in the log.
                failureDetail = reader.failureMessage().empty() ? reader.bodyPrefix(800) : reader.failureMessage();
            }
            else if (reader.overflowed())
            {
                transportFailed = true;
                transportMessage = "AI stream exceeded the 2MB response budget";
            }
            else if (!reader.failureMessage().empty())
            {
                transportFailed = true;
                transportMessage = reader.failureMessage();
            }

            if (transportFailed)
            {
                LOG_ERROR << "AiClient: stream failed: " << transportMessage
                          << (failureDetail.empty() ? "" : (" | provider body: " + failureDetail));
                result.success = false;
                result.errorMessage = transportMessage;
                if (sharedCallbacks->onError) sharedCallbacks->onError(result);
                return;
            }

            result.success = true;
            const auto toolCalls = reader.toolCalls();
            if (!toolCalls.empty() && sharedCallbacks->onToolCalls)
            {
                sharedCallbacks->onToolCalls(toolCalls);
            }
            result.toolCalls = toolCalls;
            LOG_INFO << "AiClient: stream finished, content length=" << result.content.size()
                     << ", tool calls=" << toolCalls.size();
            if (sharedCallbacks->onFinished) sharedCallbacks->onFinished(result);
        });

    if (!accepted)
    {
        AiStreamResult busyResult;
        busyResult.success = false;
        busyResult.errorMessage = "AI request queue is full";
        busyResult.modelName = model;
        busyResult.busy = true;
        if (sharedCallbacks->onError) sharedCallbacks->onError(busyResult);
    }
}

} // namespace mathai::clients
