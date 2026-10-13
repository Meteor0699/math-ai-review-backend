#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>

#include <json/json.h>

namespace mathai::utils
{

// Agent-mode payload budget (docs/AGENT_MODE.md): every tool result that is
// streamed to the client or replayed into the model context is cut to 4KB.
constexpr std::size_t agentPayloadCapBytes = 4 * 1024;

// Payloads that travel to the model (or through the SSE stream) are capped so a
// single row with a huge MEDIUMTEXT value cannot blow up the request. The cut is
// always made on a UTF-8 code point boundary, otherwise the truncated bytes are
// re-encoded as replacement characters downstream.
inline std::string truncateUtf8(const std::string &value, std::size_t maximumBytes)
{
    if (value.size() <= maximumBytes)
    {
        return value;
    }

    std::size_t end = maximumBytes;
    while (end > 0 && (static_cast<unsigned char>(value[end - 1]) & 0xC0U) == 0x80U)
    {
        --end;
    }
    return value.substr(0, end);
}

// Compact single-line JSON, used for tool_args / tool_result persistence and for
// the SSE frames.
inline std::string jsonToCompactString(const Json::Value &value)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

// Tolerant JSON parse for values that arrive from the model or from a TEXT
// column: anything unparsable is handed back as a plain string so a bad row can
// never abort a turn.
inline Json::Value parseJsonLoose(const std::string &text)
{
    if (text.empty())
    {
        return Json::Value(Json::objectValue);
    }

    Json::CharReaderBuilder builder;
    Json::Value parsed;
    std::string errors;
    const auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
    if (reader->parse(text.data(), text.data() + text.size(), &parsed, &errors))
    {
        return parsed;
    }
    return Json::Value(text);
}

} // namespace mathai::utils
