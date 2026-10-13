#include "dao/AgentDao.h"

#include <drogon/drogon.h>

#include <algorithm>
#include <sstream>
#include <utility>

#include "utils/MysqlSyncClient.h"
#include "utils/TextUtil.h"

namespace mathai::dao
{
namespace
{

std::string quote(const std::string &value) { return mathai::utils::mysql::quote(value); }

std::string nullableText(const std::string &value) { return value.empty() ? "NULL" : quote(value); }

// Rows that carry a truncated payload are marked so the model is not misled by
// a silently shortened tool result.
constexpr std::size_t storedPayloadCap = 64 * 1024;
constexpr std::size_t historyMessageCap = 40;

std::string cappedStored(const std::string &value) { return mathai::utils::truncateUtf8(value, storedPayloadCap); }

AgentMessage rowToMessage(const mathai::utils::mysql::Row &row)
{
    AgentMessage message;
    message.id = row.getInt64("id");
    message.role = row.getString("role");
    message.content = row.getString("content");
    message.toolName = row.getString("tool_name");
    message.toolArgs = row.getString("tool_args");
    message.toolResult = row.getString("tool_result");
    message.createdAt = row.getString("created_at");
    return message;
}

} // namespace

void AgentDao::findOrCreateConversation(long long userId,
                                        long long questionId,
                                        IdCallback onSuccess,
                                        ErrorCallback onError) const
{
    try
    {
        // INSERT ... ON DUPLICATE KEY UPDATE keeps the (user_id, question_id)
        // pair unique without a read-modify-write race; the id is then always
        // re-selected by that same pair so a concurrent creator cannot leak a
        // foreign conversation id into this turn.
        mathai::utils::mysql::execute(
            "INSERT INTO ai_conversation (user_id, question_id, message_count) VALUES (" +
            std::to_string(userId) + ", " + std::to_string(questionId) + ", 0) "
            "ON DUPLICATE KEY UPDATE user_id = VALUES(user_id)");

        const auto result = mathai::utils::mysql::execute(
            "SELECT id FROM ai_conversation WHERE user_id = " + std::to_string(userId) +
            " AND question_id = " + std::to_string(questionId) + " LIMIT 1");
        if (result.rows.empty())
        {
            onError("conversation row disappeared after upsert");
            return;
        }
        onSuccess(result.rows[0].getInt64("id"));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void AgentDao::findConversationIdForUser(long long userId,
                                         long long questionId,
                                         IdCallback onSuccess,
                                         ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id FROM ai_conversation WHERE user_id = " + std::to_string(userId) +
            " AND question_id = " + std::to_string(questionId) + " LIMIT 1");
        onSuccess(result.rows.empty() ? 0 : result.rows[0].getInt64("id"));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void AgentDao::appendMessage(long long conversationId,
                             const std::string &role,
                             const std::string &content,
                             const std::string &toolName,
                             const std::string &toolArgs,
                             const std::string &toolResult,
                             IdCallback onSuccess,
                             ErrorCallback onError) const
{
    try
    {
        std::ostringstream sql;
        sql << "INSERT INTO ai_message (conversation_id, role, content, tool_name, tool_args, tool_result) VALUES ("
            << conversationId << ", " << quote(role) << ", " << nullableText(cappedStored(content)) << ", "
            << nullableText(toolName) << ", " << nullableText(cappedStored(toolArgs)) << ", "
            << nullableText(cappedStored(toolResult)) << ")";
        const auto inserted = mathai::utils::mysql::execute(sql.str());

        mathai::utils::mysql::execute(
            "UPDATE ai_conversation SET message_count = message_count + 1, "
            "updated_at = GREATEST(updated_at, CURRENT_TIMESTAMP) WHERE id = " + std::to_string(conversationId));
        onSuccess(static_cast<long long>(inserted.insertId));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void AgentDao::updateMessageContent(long long messageId,
                                    const std::string &content,
                                    ErrorCallback onError) const
{
    try
    {
        const auto updated = mathai::utils::mysql::execute(
            "UPDATE ai_message SET content = " + nullableText(cappedStored(content)) +
            " WHERE id = " + std::to_string(messageId));
        if (updated.affectedRows == 0)
        {
            // Either the row is gone or the text is unchanged; both are fine for
            // the caller, which only cares that the content is durable.
            const auto exists = mathai::utils::mysql::execute(
                "SELECT id FROM ai_message WHERE id = " + std::to_string(messageId) + " LIMIT 1");
            if (exists.rows.empty())
            {
                onError("assistant message row was not found");
                return;
            }
        }

        mathai::utils::mysql::execute(
            "UPDATE ai_conversation c JOIN ai_message m ON m.conversation_id = c.id "
            "SET c.updated_at = GREATEST(c.updated_at, CURRENT_TIMESTAMP) WHERE m.id = " +
            std::to_string(messageId));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void AgentDao::loadHistory(long long conversationId, MessagesCallback onSuccess, ErrorCallback onError) const
{
    loadOrdered(conversationId, "ASC", 0, false, 0, std::move(onSuccess), std::move(onError));
}

void AgentDao::loadRecentMessages(long long conversationId,
                                  int limit,
                                  long long excludeMessageId,
                                  MessagesCallback onSuccess,
                                  ErrorCallback onError) const
{
    const auto capped = std::clamp(limit, 1, static_cast<int>(historyMessageCap));
    loadOrdered(conversationId, "DESC", capped, true, excludeMessageId, std::move(onSuccess), std::move(onError));
}

void AgentDao::loadOrdered(long long conversationId,
                           const std::string &order,
                           int limit,
                           bool reverseAfterLoad,
                           long long excludeMessageId,
                           MessagesCallback onSuccess,
                           ErrorCallback onError) const
{
    try
    {
        std::ostringstream sql;
        sql << "SELECT id, role, content, tool_name, tool_args, tool_result, created_at FROM ai_message "
            << "WHERE conversation_id = " << conversationId;
        if (excludeMessageId > 0)
        {
            sql << " AND id <> " << excludeMessageId;
        }
        if (limit > 0)
        {
            // The reserved assistant placeholder is filtered in C++, so ask for
            // a few extra rows to keep the context window full.
            sql << " AND NOT (role = 'assistant' AND (content IS NULL OR content = ''))";
        }
        sql << " ORDER BY id " << order;
        if (limit > 0)
        {
            sql << " LIMIT " << (limit + 2);
        }

        const auto result = mathai::utils::mysql::execute(sql.str());
        std::vector<AgentMessage> messages;
        messages.reserve(result.rows.size());
        for (const auto &row : result.rows)
        {
            messages.push_back(rowToMessage(row));
        }
        if (reverseAfterLoad)
        {
            std::reverse(messages.begin(), messages.end());
        }
        if (limit > 0 && static_cast<int>(messages.size()) > limit)
        {
            messages.erase(messages.begin(), messages.end() - limit);
        }
        onSuccess(std::move(messages));
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

void AgentDao::questionExists(long long questionId,
                              std::function<void(bool)> onSuccess,
                              ErrorCallback onError) const
{
    try
    {
        const auto result = mathai::utils::mysql::execute(
            "SELECT id FROM question WHERE id = " + std::to_string(questionId) + " AND status = 1 LIMIT 1");
        onSuccess(!result.rows.empty());
    }
    catch (const std::exception &exception)
    {
        onError(exception.what());
    }
}

} // namespace mathai::dao
