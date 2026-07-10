#pragma once

#include <drogon/HttpResponse.h>
#include <functional>
#include <string>

namespace mathai::clients
{

struct AiResult
{
    bool success{};
    std::string content;
    std::string errorMessage;
    std::string modelName;
    bool busy{};
};

class AiClient
{
  public:
    using Callback = std::function<void(AiResult)>;

    void generateExplanation(const std::string &prompt, Callback callback) const;
};

} // namespace mathai::clients
