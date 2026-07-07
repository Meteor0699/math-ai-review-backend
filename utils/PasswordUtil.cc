#include "utils/PasswordUtil.h"

#include <openssl/sha.h>

#include <iomanip>
#include <sstream>
#include <vector>

namespace mathai::utils
{

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

std::string makePasswordHash(const std::string &plainPassword, const std::string &salt)
{
    return "sha256$" + salt + "$" + sha256Hex(salt + ":" + plainPassword);
}

bool verifyPassword(const std::string &plainPassword, const std::string &storedHash)
{
    const auto first = storedHash.find('$');
    if (first == std::string::npos)
    {
        return false;
    }

    const auto second = storedHash.find('$', first + 1);
    if (second == std::string::npos)
    {
        return false;
    }

    const auto algorithm = storedHash.substr(0, first);
    const auto salt = storedHash.substr(first + 1, second - first - 1);
    if (algorithm != "sha256")
    {
        return false;
    }

    return makePasswordHash(plainPassword, salt) == storedHash;
}

} // namespace mathai::utils
