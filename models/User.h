#pragma once

#include <cstdint>
#include <string>

namespace mathai::models
{

struct User
{
    std::int64_t id{};
    std::string username;
    std::string passwordHash;
    std::string realName;
    std::string studentNo;
    std::string role;
    int status{};
    int authVersion{1};
};

} // namespace mathai::models
