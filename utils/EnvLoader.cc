#include "utils/EnvLoader.h"

#include <cstdlib>
#include <fstream>
#include <string>

namespace mathai::utils
{
namespace
{

std::string trim(const std::string &value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
    {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

void setEnvValue(const std::string &key, const std::string &value)
{
    if (key.empty())
    {
        return;
    }

#ifdef _WIN32
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 0);
#endif
}

} // namespace

void loadDotEnv(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string::npos)
        {
            continue;
        }

        auto key = trim(line.substr(0, eq));
        auto value = trim(line.substr(eq + 1));
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\'')))
        {
            value = value.substr(1, value.size() - 2);
        }
        setEnvValue(key, value);
    }
}

} // namespace mathai::utils
