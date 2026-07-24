#pragma once

#include <string>

namespace mathai::utils
{

std::string sha256Hex(const std::string &input);
bool verifyPassword(const std::string &plainPassword, const std::string &storedHash);
bool passwordNeedsRehash(const std::string &storedHash);
std::string makePasswordHash(const std::string &plainPassword);

} // namespace mathai::utils
