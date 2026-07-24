#include "utils/JwtUtil.h"

#include <drogon/drogon.h>
#include <json/json.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <cstdlib>
#include <ctime>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace mathai::utils
{
namespace
{

const std::string &jwtSecret()
{
    static const std::string secret = [] {
        const auto *value = std::getenv("JWT_SECRET");
        if (value == nullptr || std::string(value).size() < 32)
        {
            throw std::runtime_error("JWT_SECRET must be set to at least 32 characters");
        }
        return std::string(value);
    }();
    return secret;
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
    if (input.empty() || input.size() % 4 == 1)
    {
        return std::nullopt;
    }
    for (const auto ch : input)
    {
        const auto valid = (ch >= 'A' && ch <= 'Z') ||
                           (ch >= 'a' && ch <= 'z') ||
                           (ch >= '0' && ch <= '9') || ch == '-' || ch == '_';
        if (!valid)
        {
            return std::nullopt;
        }
    }

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

void validateJwtConfiguration()
{
    (void)jwtSecret();
}

std::string createJwt(std::int64_t userId,
                      const std::string &username,
                      const std::string &role,
                      int authVersion)
{
    if (userId <= 0 || username.empty() || authVersion < 1 ||
        (role != "student" && role != "admin"))
    {
        throw std::invalid_argument("invalid JWT claims");
    }

    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    const auto now = static_cast<std::int64_t>(std::time(nullptr));
    Json::Value payload;
    payload["sub"] = Json::Int64(userId);
    payload["username"] = username;
    payload["role"] = role;
    payload["ver"] = authVersion;
    payload["iat"] = Json::Int64(now);
    payload["exp"] = Json::Int64(now + jwtExpireHours() * 3600);

    const auto signingInput =
        base64UrlEncode(jsonToString(header)) + "." + base64UrlEncode(jsonToString(payload));
    return signingInput + "." + base64UrlEncode(hmacSha256(signingInput, jwtSecret()));
}

std::optional<JwtClaims> verifyJwt(const std::string &token)
{
    if (token.empty() || token.size() > 8192)
    {
        return std::nullopt;
    }

    const auto first = token.find('.');
    const auto second = token.find('.', first == std::string::npos ? 0 : first + 1);
    if (first == std::string::npos || second == std::string::npos ||
        first == 0 || second == first + 1 || second + 1 == token.size() ||
        token.find('.', second + 1) != std::string::npos)
    {
        return std::nullopt;
    }

    const auto signingInput = token.substr(0, second);
    const auto expectedSignature = base64UrlEncode(hmacSha256(signingInput, jwtSecret()));
    const auto providedSignature = token.substr(second + 1);
    if (expectedSignature.size() != providedSignature.size() ||
        CRYPTO_memcmp(expectedSignature.data(), providedSignature.data(), expectedSignature.size()) != 0)
    {
        return std::nullopt;
    }

    const auto headerText = base64UrlDecode(token.substr(0, first));
    if (!headerText)
    {
        return std::nullopt;
    }
    const auto header = stringToJson(*headerText);
    if (!header || !header->isObject() ||
        !(*header)["alg"].isString() || !(*header)["typ"].isString() ||
        (*header)["alg"].asString() != "HS256" ||
        (*header)["typ"].asString() != "JWT")
    {
        return std::nullopt;
    }

    const auto payloadText = base64UrlDecode(token.substr(first + 1, second - first - 1));
    if (!payloadText)
    {
        return std::nullopt;
    }

    const auto payload = stringToJson(*payloadText);
    if (!payload || !payload->isObject() ||
        !(*payload)["sub"].isIntegral() || !(*payload)["role"].isString() ||
        !(*payload)["username"].isString() || !(*payload)["ver"].isIntegral() ||
        !(*payload)["exp"].isIntegral())
    {
        return std::nullopt;
    }

    JwtClaims claims;
    claims.userId = (*payload)["sub"].asInt64();
    claims.username = (*payload)["username"].asString();
    claims.role = (*payload)["role"].asString();
    claims.authVersion = (*payload)["ver"].asInt();
    claims.exp = (*payload)["exp"].asInt64();

    if (claims.userId <= 0 || claims.username.empty() || claims.authVersion < 1 ||
        (claims.role != "student" && claims.role != "admin") ||
        claims.exp <= static_cast<std::int64_t>(std::time(nullptr)))
    {
        return std::nullopt;
    }
    return claims;
}

} // namespace mathai::utils
