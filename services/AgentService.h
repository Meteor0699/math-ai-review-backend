#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <json/json.h>

#include "clients/AiClient.h"
#include "dao/AgentDao.h"

namespace mathai::services
{

// Cancellation flag shared between the SSE controller (which flips it when the
// client disconnects) and the AI worker thread.
using AgentCancelFlag = std::shared_ptr<std::atomic_bool>;

constexpr int agentMaximumModelCalls = 5;

struct AgentTurnRequest
{
    long long userId{0};
    long long questionId{0};
    // 0 means "reuse or create the conversation for (userId, questionId)".
    long long conversationId{0};
    std::string message;
};

struct AgentTurnCallbacks
{
    // First frame: conversation id plus both message ids. `assistantMessageId`
    // is a placeholder row reserved before generation, so it is valid from here
    // on; its content is filled in when the turn settles.
    std::function<void(long long conversationId, long long userMessageId, long long assistantMessageId)> onMeta;
    std::function<void(const std::string &)> onDelta;
    std::function<void(const std::string &name, const Json::Value &args)> onToolStart;
    std::function<void(const std::string &name, const Json::Value &result, long long milliseconds)> onToolDone;
    std::function<void(long long assistantMessageId, int promptTokens, int completionTokens)> onDone;
    // code is one of busy / ai_unavailable / database_error / invalid_request /
    // not_found / internal_error.
    std::function<void(const std::string &message, const std::string &code)> onError;
};

// Snapshot handed to the controller when a turn has been accepted: the frames
// in it must be emitted before the model work starts.
struct AgentTurnHandle
{
    bool ok{false};
    // Set when a pre-generation step failed and no stream should be started.
    std::string errorMessage;
    std::string errorCode;
    // Conversation id, user message id and the reserved assistant message id.
    long long conversationId{0};
    long long userMessageId{0};
    long long assistantMessageId{0};
};

class AgentService
{
  public:
    // Full history for GET .../agent/session. `conversationId` is 0 when the
    // user has never talked about this question; nothing is written.
    void loadSession(long long userId,
                     long long questionId,
                     std::function<void(long long conversationId, Json::Value messages)> onSuccess,
                     std::function<void(const std::string &)> onError) const;

    // Validates the request, resolves the conversation, and persists the user
    // message plus a reserved assistant row. Runs on the HTTP thread (fast
    // synchronous queries); the model work happens in generateTurn.
    AgentTurnHandle prepareTurn(const AgentTurnRequest &request, const AgentTurnCallbacks &callbacks) const;

    // Shared AI queue capacity probe, so a full queue answers `busy` without
    // persisting a turn that can never be generated.
    static bool hasCapacity();

    // Runs the full agent loop on a dedicated per-turn orchestrator thread;
    // individual model calls are submitted to the shared AiExecutor.
    // `excludedUserMessageId` is the user message just written, kept out of the
    // rebuilt model context because the caller replays it. The turn always
    // settles exactly once (onDone or onError) unless the client cancelled, in
    // which case the partial answer is persisted and neither callback fires.
    void generateTurn(const AgentTurnRequest &request,
                      long long conversationId,
                      long long excludedUserMessageId,
                      long long assistantMessageId,
                      const AgentTurnCallbacks &callbacks,
                      AgentCancelFlag cancel) const;

    // The tools the model may call, in OpenAI tools-schema form.
    static Json::Value toolSchema();

    // Submits one model round to the shared AiExecutor. Public so the turn
    // runner (a free function in the .cc) can drive the round chain.
    void submitStream(const Json::Value &messages,
                      const Json::Value &tools,
                      mathai::clients::AiStreamCallbacks callbacks) const;

  private:
    mathai::dao::AgentDao agentDao_;
    mathai::clients::AiClient aiClient_;
};

} // namespace mathai::services
