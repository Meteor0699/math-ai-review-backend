#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <optional>

#include "utils/JwtUtil.h"

namespace mathai::utils
{

enum class AuthState
{
    Ok,
    Missing,
    Invalid,
    DatabaseError
};

struct AuthResult
{
    AuthState state{AuthState::Invalid};
    std::optional<JwtClaims> claims;
};

AuthResult authenticateRequest(const drogon::HttpRequestPtr &request);
drogon::HttpResponsePtr authErrorResponse(AuthState state);

} // namespace mathai::utils
