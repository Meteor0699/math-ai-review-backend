#pragma once

#include <functional>
#include <optional>
#include <string>
#include <cstdint>

#include "models/User.h"
#include <json/json.h>

namespace mathai::dao
{

class UserDao
{
  public:
    using UserCallback = std::function<void(std::optional<mathai::models::User>)>;
    using JsonCallback = std::function<void(Json::Value)>;
    using CountCallback = std::function<void(std::uint64_t)>;
    using ErrorCallback = std::function<void(const std::string &)>;

    void findByUsername(const std::string &username,
                        UserCallback onSuccess,
                        ErrorCallback onError) const;

    void list(const std::string &keyword,
              const std::string &role,
              int page,
              int pageSize,
              JsonCallback onSuccess,
              ErrorCallback onError) const;

    void create(const Json::Value &user,
                JsonCallback onSuccess,
                ErrorCallback onError) const;

    void update(long long id,
                const Json::Value &user,
                CountCallback onSuccess,
                ErrorCallback onError) const;

    void updatePassword(long long id,
                        const std::string &passwordHash,
                        CountCallback onSuccess,
                        ErrorCallback onError) const;

    void recordSuccessfulLogin(long long id,
                               int authVersion,
                               const std::string &upgradedPasswordHash,
                               CountCallback onSuccess,
                               ErrorCallback onError) const;

    void disable(long long id,
                 CountCallback onSuccess,
                 ErrorCallback onError) const;
};

} // namespace mathai::dao
