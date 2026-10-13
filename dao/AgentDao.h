#pragma once

#include <functional>
#include <string>
#include <vector>

#include <json/json.h>

namespace mathai::dao
{

// One persisted row of ai_message, in the shape both the detail endpoint and the
// model-context builder need.
struct AgentMessage
{
    long long id{0};
    std::string role;
    std::string content;
    std::string toolName;
    std::string toolArgs;
    std::string toolResult;
    std::string createdAt;
};

class AgentDao
{
  public:
    using JsonCallback = std::function<void(Json::Value)>;
    using IdCallback = std::function<void(long long)>;
    using MessagesCallback = std::function<void(std::vector<AgentMessage>)>;
    using ErrorCallback = std::function<void(const std::string &)>;

    // Returns the existing conversation id for (userId, questionId) or creates
    // one. Never returns the id of another user's conversation: the unique key
    // is (user_id, question_id) and the id is re-selected by that same pair.
    void findOrCreateConversation(long long userId,
                                  long long questionId,
                                  IdCallback onSuccess,
                                  ErrorCallback onError) const;

    // Conversation id only when it belongs to userId, otherwise 0.
    void findConversationIdForUser(long long userId,
                                   long long questionId,
                                   IdCallback onSuccess,
                                   ErrorCallback onError) const;

    // Appends one message and bumps message_count / updated_at.
    void appendMessage(long long conversationId,
                       const std::string &role,
                       const std::string &content,
                       const std::string &toolName,
                       const std::string &toolArgs,
                       const std::string &toolResult,
                       IdCallback onSuccess,
                       ErrorCallback onError) const;

    // Overwrites the content of a previously reserved assistant row (used for
    // the streaming placeholder) and bumps the conversation counters.
    void updateMessageContent(long long messageId,
                              const std::string &content,
                              ErrorCallback onError) const;

    // Full ordered history for GET .../agent/session.
    void loadHistory(long long conversationId, MessagesCallback onSuccess, ErrorCallback onError) const;

    // Newest `limit` messages in ascending id order (model context window).
    // `excludeMessageId` drops the row just written for the current turn, and
    // reserved assistant placeholders (empty content) are always skipped, so the
    // rebuilt context never contains the in-flight question twice.
    void loadRecentMessages(long long conversationId,
                            int limit,
                            long long excludeMessageId,
                            MessagesCallback onSuccess,
                            ErrorCallback onError) const;

    // True when the question exists and is visible to students.
    void questionExists(long long questionId, std::function<void(bool)> onSuccess, ErrorCallback onError) const;

  private:
    void loadOrdered(long long conversationId,
                     const std::string &order,
                     int limit,
                     bool reverseAfterLoad,
                     long long excludeMessageId,
                     MessagesCallback onSuccess,
                     ErrorCallback onError) const;
};

} // namespace mathai::dao
