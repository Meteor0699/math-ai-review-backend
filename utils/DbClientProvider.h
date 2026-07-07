#pragma once

#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>

namespace mathai::utils
{

inline drogon::orm::DbClientPtr db()
{
    return drogon::app().getFastDbClient("default");
}

} // namespace mathai::utils
