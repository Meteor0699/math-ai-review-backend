#include "utils/PasswordUtil.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <array>
#include <charconv>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace mathai::utils
{
namespace
{

constexpr int pbkdf2Iterations = 210000;
constexpr std::size_t saltSize = 16;
constexpr std::size_t derivedKeySize = 32;

std::string hexEncode(const unsigned char *data, std::size_t size)
{
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < size; ++index)
    {
        output << std::setw(2) << static_cast<int>(data[index]);
    }
    return output.str();
}

std::optional<std::vector<unsigned char>> hexDecode(const std::string &value)
{
    if (value.empty() || value.size() % 2 != 0)
    {
        return std::nullopt;
    }

    std::vector<unsigned char> bytes(value.size() / 2);
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
        unsigned int byte{};
        const auto *begin = value.data() + index * 2;
        const auto result = std::from_chars(begin, begin + 2, byte, 16);
        if (result.ec != std::errc{} || result.ptr != begin + 2 || byte > 0xff)
        {
            return std::nullopt;
        }
        bytes[index] = static_cast<unsigned char>(byte);
    }
    return bytes;
}

bool constantTimeEqual(const std::string &left, const std::string &right)
{
    return left.size() == right.size() &&
           CRYPTO_memcmp(left.data(), right.data(), left.size()) == 0;
}

} // namespace

std::string sha256Hex(const std::string &input)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), digest);

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const auto byte : digest)
    {
        output << std::setw(2) << static_cast<int>(byte);
    }
    return output.str();
}

std::string makePasswordHash(const std::string &plainPassword)
{
    std::array<unsigned char, saltSize> salt{};
    std::array<unsigned char, derivedKeySize> derivedKey{};
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1 ||
        PKCS5_PBKDF2_HMAC(plainPassword.data(), static_cast<int>(plainPassword.size()),
                          salt.data(), static_cast<int>(salt.size()), pbkdf2Iterations,
                          EVP_sha256(), static_cast<int>(derivedKey.size()), derivedKey.data()) != 1)
    {
        throw std::runtime_error("failed to hash password");
    }

    return "pbkdf2_sha256$" + std::to_string(pbkdf2Iterations) + "$" +
           hexEncode(salt.data(), salt.size()) + "$" +
           hexEncode(derivedKey.data(), derivedKey.size());
}

bool verifyPassword(const std::string &plainPassword, const std::string &storedHash)
{
    if (storedHash.rfind("sha256$", 0) == 0)
    {
        const auto saltEnd = storedHash.find('$', 7);
        if (saltEnd == std::string::npos) return false;
        const auto salt = storedHash.substr(7, saltEnd - 7);
        const auto expected = "sha256$" + salt + "$" + sha256Hex(salt + ":" + plainPassword);
        return constantTimeEqual(expected, storedHash);
    }

    if (storedHash.rfind("pbkdf2_sha256$", 0) != 0)
    {
        return false;
    }

    const auto iterationEnd = storedHash.find('$', 14);
    const auto saltEnd = iterationEnd == std::string::npos ? std::string::npos : storedHash.find('$', iterationEnd + 1);
    if (iterationEnd == std::string::npos || saltEnd == std::string::npos)
    {
        return false;
    }

    int iterations{};
    const auto iterationText = storedHash.substr(14, iterationEnd - 14);
    const auto parsedIterations = std::from_chars(iterationText.data(), iterationText.data() + iterationText.size(), iterations);
    if (parsedIterations.ec != std::errc{} || parsedIterations.ptr != iterationText.data() + iterationText.size() ||
        iterations < 100000 || iterations > 1000000)
    {
        return false;
    }

    const auto salt = hexDecode(storedHash.substr(iterationEnd + 1, saltEnd - iterationEnd - 1));
    const auto expectedKey = hexDecode(storedHash.substr(saltEnd + 1));
    if (!salt || !expectedKey || salt->size() < 8 || expectedKey->size() != derivedKeySize)
    {
        return false;
    }

    std::array<unsigned char, derivedKeySize> actualKey{};
    if (PKCS5_PBKDF2_HMAC(plainPassword.data(), static_cast<int>(plainPassword.size()),
                          salt->data(), static_cast<int>(salt->size()), iterations,
                          EVP_sha256(), static_cast<int>(actualKey.size()), actualKey.data()) != 1)
    {
        return false;
    }
    return CRYPTO_memcmp(actualKey.data(), expectedKey->data(), actualKey.size()) == 0;
}

bool passwordNeedsRehash(const std::string &storedHash)
{
    return storedHash.rfind("pbkdf2_sha256$" + std::to_string(pbkdf2Iterations) + "$", 0) != 0;
}

} // namespace mathai::utils
