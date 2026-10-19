#include "controllers/AgentController.h"

#include <drogon/drogon.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "clients/AiClient.h"
#include "utils/AuthContext.h"
#include "utils/JsonResponse.h"
#include "utils/TextUtil.h"

namespace
{

constexpr const char *sseSeparator = "\n\n";

std::string frame(const std::string &event, const Json::Value &data)
{
    std::string text;
    text.reserve(64);
    text += "event: ";
    text += event;
    text += "\ndata: ";
    text += mathai::utils::jsonToCompactString(data);
    text += sseSeparator;
    return text;
}

std::string errorFrame(const std::string &message, const std::string &code)
{
    Json::Value data(Json::objectValue);
    data["message"] = message;
    data["code"] = code;
    return frame("error", data);
}

// docs/AGENT_MODE.md fixes these three headers; Drogon's async stream response
// otherwise defaults to text/html, which makes browsers treat the body as a
// document instead of an event stream.
void addSseHeaders(const drogon::HttpResponsePtr &response)
{
    response->setContentTypeString("text/event-stream; charset=utf-8");
    response->addHeader("Cache-Control", "no-cache");
    response->addHeader("X-Accel-Buffering", "no");
}

// One live SSE response. Drogon hands the stream over on the event loop thread
// and only that thread may write to or close it, so every mutation goes through
// queueInLoop. The AI worker threads see just this shared state; the cancel flag
// is itself shared because the service hands it to the model round as well.
struct AgentStreamState
{
    drogon::ResponseStreamPtr stream;
    mathai::services::AgentCancelFlag cancel = std::make_shared<std::atomic_bool>(false);
    std::atomic_bool closed{false};
};

using AgentStreamStatePtr = std::shared_ptr<AgentStreamState>;

// A stream is only ever sent to the event loop, and the registry is the single
// authority on "one stream per (user, question)".
class AgentStreamRegistry
{
  public:
    static AgentStreamRegistry &instance()
    {
        static AgentStreamRegistry registry;
        return registry;
    }

    // Returns false when this (user, question) pair already has an active turn.
    bool tryAcquire(long long userId, long long questionId)
    {
        const auto key = makeKey(userId, questionId);
        std::lock_guard<std::mutex> lock(mutex_);
        return active_.emplace(key, true).second;
    }

    void release(long long userId, long long questionId)
    {
        const auto key = makeKey(userId, questionId);
        std::lock_guard<std::mutex> lock(mutex_);
        active_.erase(key);
    }

  private:
    static std::string makeKey(long long userId, long long questionId)
    {
        return std::to_string(userId) + ":" + std::to_string(questionId);
    }

    std::mutex mutex_;
    std::unordered_map<std::string, bool> active_;
};

class AgentStream
{
  public:
    AgentStream(AgentStreamStatePtr state, long long userId, long long questionId)
        : state_(std::move(state)), userId_(userId), questionId_(questionId)
    {
    }

    void send(const std::string &payload)
    {
        auto state = state_;
        const auto body = std::make_shared<std::string>(payload);
        drogon::app().getLoop()->queueInLoop([state, body]() {
            if (state->closed.load() || !state->stream)
            {
                return;
            }
            if (!state->stream->send(*body))
            {
                // The peer is gone. Flip the cancellation flag so the AI worker
                // stops at its next chunk boundary, and close the stream so the
                // chunked transfer is terminated.
                state->cancel->store(true);
                state->closed.store(true);
                state->stream->close();
                state->stream.reset();
            }
        });
    }

    // Terminates the SSE response exactly once, always from the event loop.
    void finish()
    {
        auto state = state_;
        drogon::app().getLoop()->queueInLoop([state]() {
            if (state->closed.exchange(true))
            {
                return;
            }
            if (state->stream)
            {
                state->stream->close();
                state->stream.reset();
            }
        });
    }

  private:
    AgentStreamStatePtr state_;
    long long userId_{0};
    long long questionId_{0};
};

std::string stringField(const Json::Value &body, const char *field)
{
    if (!body.isMember(field) || !body[field].isString())
    {
        return "";
    }
    return body[field].asString();
}

bool parseConversationId(const Json::Value &body, long long &value)
{
    if (!body.isMember("conversationId") || body["conversationId"].isNull())
    {
        value = 0;
        return true;
    }
    const auto &field = body["conversationId"];
    if (field.isIntegral())
    {
        value = field.asInt64();
        return value >= 0;
    }
    if (field.isString())
    {
        try
        {
            value = std::stoll(field.asString());
            return value >= 0;
        }
        catch (const std::exception &)
        {
            return false;
        }
    }
    return false;
}

} // namespace

void AgentController::getSession(const drogon::HttpRequestPtr &request,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 long long questionId)
{
    const auto auth = mathai::utils::authenticateRequest(request);
    if (auth.state != mathai::utils::AuthState::Ok || !auth.claims)
    {
        callback(mathai::utils::authErrorResponse(auth.state));
        return;
    }

    if (questionId <= 0)
    {
        callback(mathai::utils::error(400, "invalid question id", drogon::k400BadRequest));
        return;
    }

    auto cb = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(callback));
    agentService_.loadSession(
        auth.claims->userId,
        questionId,
        [cb](long long conversationId, Json::Value messages) {
            Json::Value data(Json::objectValue);
            data["conversationId"] = conversationId == 0 ? Json::Value(Json::nullValue)
                                                         : Json::Value(Json::Int64(conversationId));
            data["messages"] = messages;
            (*cb)(mathai::utils::jsonResponse(200, "success", data));
        },
        [cb](const std::string &message) {
            LOG_ERROR << "AgentController: session lookup failed: " << message;
            (*cb)(mathai::utils::jsonResponse(500, "database error", Json::Value(Json::objectValue),
                                              drogon::k500InternalServerError));
        });
}

void AgentController::chat(const drogon::HttpRequestPtr &request,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                           long long questionId)
{
    const auto auth = mathai::utils::authenticateRequest(request);
    if (auth.state != mathai::utils::AuthState::Ok || !auth.claims)
    {
        callback(mathai::utils::authErrorResponse(auth.state));
        return;
    }
    const auto userId = auth.claims->userId;

    if (questionId <= 0)
    {
        callback(mathai::utils::error(400, "invalid question id", drogon::k400BadRequest));
        return;
    }

    // Every JSON response this endpoint can legitimately produce is an error:
    // 200 is always text/event-stream, so the client can tell them apart by
    // status code.
    const auto jsonFailure = [&callback](int code, const std::string &message, drogon::HttpStatusCode status) {
        callback(mathai::utils::error(code, message, status));
    };

    const auto body = request->getJsonObject();
    if (!body || !body->isObject())
    {
        jsonFailure(400, "invalid JSON body", drogon::k400BadRequest);
        return;
    }

    const auto message = stringField(*body, "message");
    if (message.empty())
    {
        jsonFailure(400, "message is required", drogon::k400BadRequest);
        return;
    }

    long long requestedConversationId = 0;
    if (!parseConversationId(*body, requestedConversationId))
    {
        jsonFailure(400, "invalid conversationId", drogon::k400BadRequest);
        return;
    }

    // The whole turn is serialized per (user, question): a second concurrent
    // stream is answered with `event: error` + `code: busy`.
    if (!AgentStreamRegistry::instance().tryAcquire(userId, questionId))
    {
        auto response = drogon::HttpResponse::newAsyncStreamResponse(
            [](drogon::ResponseStreamPtr stream) {
                if (!stream)
                {
                    return;
                }
                stream->send(errorFrame("another agent stream is already running for this question", "busy"));
                stream->close();
            },
            true);
        addSseHeaders(response);
        callback(response);
        return;
    }

    mathai::services::AgentTurnRequest turnRequest;
    turnRequest.userId = userId;
    turnRequest.questionId = questionId;
    turnRequest.conversationId = requestedConversationId;
    turnRequest.message = message;

    auto released = std::make_shared<std::atomic_bool>(false);
    const auto release = [userId, questionId, released]() {
        if (!released->exchange(true))
        {
            AgentStreamRegistry::instance().release(userId, questionId);
        }
    };

    auto response = drogon::HttpResponse::newAsyncStreamResponse(
        [this, turnRequest, release](drogon::ResponseStreamPtr stream) {
            if (!stream)
            {
                // The client disconnected before the turn could start.
                release();
                return;
            }

            auto state = std::make_shared<AgentStreamState>();
            state->stream = std::move(stream);
            auto agentStream = std::make_shared<AgentStream>(state, turnRequest.userId, turnRequest.questionId);
            auto callbacks = std::make_shared<mathai::services::AgentTurnCallbacks>();
            auto turnAccepted = std::make_shared<bool>(false);

            callbacks->onMeta = [agentStream](long long conversationId, long long userMessageId,
                                              long long assistantMessageId) {
                Json::Value data(Json::objectValue);
                data["conversationId"] = Json::Int64(conversationId);
                data["userMessageId"] = Json::Int64(userMessageId);
                data["assistantMessageId"] = Json::Int64(assistantMessageId);
                agentStream->send(frame("meta", data));
            };
            callbacks->onDelta = [agentStream](const std::string &text) {
                Json::Value data(Json::objectValue);
                data["text"] = text;
                agentStream->send(frame("delta", data));
            };
            callbacks->onToolStart = [agentStream](const std::string &name, const Json::Value &args) {
                Json::Value data(Json::objectValue);
                data["name"] = name;
                data["status"] = "running";
                data["args"] = args;
                agentStream->send(frame("tool", data));
            };
            callbacks->onToolDone = [agentStream](const std::string &name, const Json::Value &result,
                                                  long long milliseconds) {
                Json::Value data(Json::objectValue);
                data["name"] = name;
                data["status"] = "done";
                data["result"] = result;
                data["ms"] = Json::Int64(milliseconds);
                agentStream->send(frame("tool", data));
            };
            callbacks->onDone = [agentStream, release](long long assistantMessageId, int promptTokens,
                                                       int completionTokens) {
                Json::Value data(Json::objectValue);
                data["assistantMessageId"] = Json::Int64(assistantMessageId);
                data["usage"]["promptTokens"] = promptTokens;
                data["usage"]["completionTokens"] = completionTokens;
                agentStream->send(frame("done", data));
                agentStream->finish();
                release();
            };
            callbacks->onError = [agentStream, release](const std::string &message, const std::string &code) {
                agentStream->send(errorFrame(message, code));
                agentStream->finish();
                release();
            };

            const auto handle = agentService_.prepareTurn(turnRequest, *callbacks);
            if (!handle.ok)
            {
                // prepareTurn already emitted the error frame.
                agentStream->finish();
                release();
                return;
            }

            *turnAccepted = true;
            callbacks->onMeta(handle.conversationId, handle.userMessageId, handle.assistantMessageId);

            // generateTurn only submits the work and returns; the model call
            // itself runs on the AiExecutor worker, never on this IO thread.
            agentService_.generateTurn(turnRequest, handle.conversationId, handle.userMessageId,
                                       handle.assistantMessageId, *callbacks, state->cancel);
        },
        true);
    addSseHeaders(response);
    callback(response);
}
