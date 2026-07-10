#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <drogon/HttpRequest.h>

namespace mathai::utils
{

inline std::optional<std::int64_t> parseInteger(
    const std::string &text,
    std::int64_t minimum = std::numeric_limits<std::int64_t>::min(),
    std::int64_t maximum = std::numeric_limits<std::int64_t>::max())
{
    if (text.empty())
    {
        return std::nullopt;
    }

    std::int64_t value{};
    const auto *begin = text.data();
    const auto *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end || value < minimum || value > maximum)
    {
        return std::nullopt;
    }
    return value;
}

inline bool validOptionalIntegerParameter(
    const drogon::HttpRequestPtr &request,
    const std::string &name,
    std::int64_t minimum,
    std::int64_t maximum)
{
    const auto value = request->getParameter(name);
    return value.empty() || parseInteger(value, minimum, maximum).has_value();
}

} // namespace mathai::utils
