#include "utils/JwtUtil.h"

#include <drogon/drogon.h>
#include <json/json.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <ctime>
#include <memory>
#include <sstream>
#include <vector>

namespace mathai::utils
{
namespace
{

std::string jwtSecret()
{
    const auto &config = drogon::app().getCustomConfig();
    if (config.isMember("jwt_secret") && config["jwt_secret"].isString())
    {
        return config["jwt_secret"].asString();
    }
    return "change_this_secret_in_local_development";
}

int jwtExpireHours()
{
    const auto &config = drogon::app().getCustomConfig();
    if (config.isMember("jwt_expire_hours") && config["jwt_expire_hours"].isInt())
    {
        return config["jwt_expire_hours"].asInt();
    }
    return 24;
}

std::string jsonToString(const Json::Value &value)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

std::optional<Json::Value> stringToJson(const std::string &input)
{
    Json::CharReaderBuilder builder;
    Json::Value value;
    std::string errors;
    std::istringstream stream(input);
    if (!Json::parseFromStream(builder, stream, &value, &errors))
    {
        return std::nullopt;
    }
    return value;
}

std::string base64UrlEncode(const std::string &input)
{
    const auto outputLength = 4 * ((input.size() + 2) / 3);
    std::string output(outputLength, '\0');
    EVP_EncodeBlock(reinterpret_cast<unsigned char *>(&output[0]),
                    reinterpret_cast<const unsigned char *>(input.data()),
                    static_cast<int>(input.size()));

    while (!output.empty() && output.back() == '=')
    {
        output.pop_back();
    }
    for (auto &ch : output)
    {
        if (ch == '+')
        {
            ch = '-';
        }
        else if (ch == '/')
        {
            ch = '_';
        }
    }
    return output;
}

std::optional<std::string> base64UrlDecode(std::string input)
{
    for (auto &ch : input)
    {
        if (ch == '-')
        {
            ch = '+';
        }
        else if (ch == '_')
        {
            ch = '/';
        }
    }

    while (input.size() % 4 != 0)
    {
        input.push_back('=');
    }

    std::string output((input.size() / 4) * 3, '\0');
    const auto length = EVP_DecodeBlock(reinterpret_cast<unsigned char *>(&output[0]),
                                        reinterpret_cast<const unsigned char *>(input.data()),
                                        static_cast<int>(input.size()));
    if (length < 0)
    {
        return std::nullopt;
    }

    auto padding = 0;
    if (!input.empty() && input[input.size() - 1] == '=')
    {
        ++padding;
    }
    if (input.size() > 1 && input[input.size() - 2] == '=')
    {
        ++padding;
    }
    output.resize(static_cast<std::size_t>(length - padding));
    return output;
}

std::string hmacSha256(const std::string &data, const std::string &secret)
{
    unsigned int length = 0;
    unsigned char digest[EVP_MAX_MD_SIZE];
    HMAC(EVP_sha256(),
         secret.data(),
         static_cast<int>(secret.size()),
         reinterpret_cast<const unsigned char *>(data.data()),
         data.size(),
         digest,
         &length);
    return std::string(reinterpret_cast<char *>(digest), length);
}

} // namespace

std::string createJwt(std::int64_t userId,
                      const std::string &username,
                      const std::string &role)
{
    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    const auto now = static_cast<std::int64_t>(std::time(nullptr));
    Json::Value payload;
    payload["sub"] = Json::Int64(userId);
    payload["username"] = username;
    payload["role"] = role;
    payload["iat"] = Json::Int64(now);
    payload["exp"] = Json::Int64(now + jwtExpireHours() * 3600);

    const auto signingInput =
        base64UrlEncode(jsonToString(header)) + "." + base64UrlEncode(jsonToString(payload));
    return signingInput + "." + base64UrlEncode(hmacSha256(signingInput, jwtSecret()));
}

std::optional<JwtClaims> verifyJwt(const std::string &token)
{
    const auto first = token.find('.');
    const auto second = token.find('.', first == std::string::npos ? 0 : first + 1);
    if (first == std::string::npos || second == std::string::npos)
    {
        return std::nullopt;
    }

    const auto signingInput = token.substr(0, second);
    const auto expectedSignature = base64UrlEncode(hmacSha256(signingInput, jwtSecret()));
    if (expectedSignature != token.substr(second + 1))
    {
        return std::nullopt;
    }

    const auto payloadText = base64UrlDecode(token.substr(first + 1, second - first - 1));
    if (!payloadText)
    {
        return std::nullopt;
    }

    const auto payload = stringToJson(*payloadText);
    if (!payload || !payload->isMember("sub") || !payload->isMember("role") ||
        !payload->isMember("username") || !payload->isMember("exp"))
    {
        return std::nullopt;
    }

    JwtClaims claims;
    claims.userId = (*payload)["sub"].asInt64();
    claims.username = (*payload)["username"].asString();
    claims.role = (*payload)["role"].asString();
    claims.exp = (*payload)["exp"].asInt64();

    if (claims.exp < static_cast<std::int64_t>(std::time(nullptr)))
    {
        return std::nullopt;
    }
    return claims;
}

} // namespace mathai::utils
