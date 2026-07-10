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
    int authVersion{1};
    std::int64_t exp{};
};

void validateJwtConfiguration();

std::string createJwt(std::int64_t userId,
                      const std::string &username,
                      const std::string &role,
                      int authVersion);

std::optional<JwtClaims> verifyJwt(const std::string &token);

} // namespace mathai::utils
