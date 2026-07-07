#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace mathai::utils
{

struct JwtClaims
{
    std::int64_t userId{};
    std::string username;
    std::string role;
    std::int64_t exp{};
};

std::string createJwt(std::int64_t userId,
                      const std::string &username,
                      const std::string &role);

std::optional<JwtClaims> verifyJwt(const std::string &token);

} // namespace mathai::utils
