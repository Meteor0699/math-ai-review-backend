#pragma once

#include <functional>
#include <optional>
#include <string>

#include <json/json.h>

namespace mathai::dao
{

class AiExplanationDao
{
  public:
    using JsonCallback = std::function<void(Json::Value)>;
    using ErrorCallback = std::function<void(const std::string &)>;

    void findExisting(long long questionId, JsonCallback onSuccess, ErrorCallback onError) const;
    void findQuestionContext(long long questionId, JsonCallback onSuccess, ErrorCallback onError) const;
    void saveSuccess(long long questionId,
                     const std::string &modelName,
                     const std::string &prompt,
                     const std::string &explanation,
                     JsonCallback onSuccess,
                     ErrorCallback onError) const;
};

} // namespace mathai::dao
