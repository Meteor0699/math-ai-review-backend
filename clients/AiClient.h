#pragma once

#include <drogon/HttpResponse.h>
#include <functional>
#include <string>
#include <vector>

#include <json/json.h>

namespace mathai::clients
{

struct AiResult
{
    bool success{};
    std::string content;
    std::string errorMessage;
    std::string modelName;
    bool busy{};
};

// One tool call requested by the model, already reassembled from the SSE deltas.
struct AiToolCall
{
    std::string id;
    std::string name;
    std::string arguments;
};

// Accumulated outcome of one streaming model call.
struct AiStreamResult
{
    bool success{};
    // Concatenation of every choices[0].delta.content chunk.
    std::string content;
    // Concatenation of every choices[0].delta.reasoning_content chunk. Thinking
    // models require this to be replayed on the assistant message that carries
    // tool calls, otherwise the provider rejects the follow-up request.
    std::string reasoningContent;
    std::vector<AiToolCall> toolCalls;
    std::string finishReason;
    int promptTokens{0};
    int completionTokens{0};
    std::string errorMessage;
    std::string modelName;
    bool busy{};
};

struct AiStreamCallbacks
{
    // Incremental assistant text, called as bytes arrive on the AI worker.
    std::function<void(const std::string &)> onDelta;
    // Called once, after the stream ends, when the model asked for tools.
    std::function<void(const std::vector<AiToolCall> &)> onToolCalls;
    std::function<void(const AiStreamResult &)> onFinished;
    std::function<void(const AiStreamResult &)> onError;
    // Polled between chunks: once it returns true the transfer is aborted and
    // the partial result is reported through onFinished.
    std::function<bool()> cancelled;
};

class AiClient
{
  public:
    using Callback = std::function<void(AiResult)>;

    std::string modelName() const;
    void generateExplanation(const std::string &prompt, Callback callback) const;

    // OpenAI-compatible streaming call with optional function calling.
    // `messages` is the full chat history array; `tools` is an OpenAI tools
    // schema array (empty disables tool calling, and the `tools` member is then
    // omitted from the request entirely). Never blocks the calling thread: the
    // transfer runs on the shared AiExecutor, so the same queue bound and
    // AI_MAX_CONCURRENCY limit as generateExplanation apply.
    void generateStream(const Json::Value &messages,
                        const Json::Value &tools,
                        AiStreamCallbacks callbacks) const;

    // False when the shared AI work queue is full. Callers use this to answer
    // `code: busy` without persisting a turn that cannot be generated.
    static bool hasCapacity();
};

} // namespace mathai::clients
