#pragma once

#include <cstdint>
#include <string>

namespace mathai::models
{

struct Course
{
    std::int64_t id{};
    std::string name;
    std::string code;
};

struct Chapter
{
    std::int64_t id{};
    std::int64_t courseId{};
    std::string title;
};

} // namespace mathai::models
