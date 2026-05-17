#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "wfrest/HttpServer.h"

inline void set_json_response(wfrest::HttpResp *resp, const nlohmann::json &data)
{
    resp->headers["Content-Type"] = "application/json; charset=utf-8";
    resp->String(data.dump());
}

inline nlohmann::json json_success(const nlohmann::json &data = nullptr)
{
    return {{"success", true}, {"data", data}};
}

inline nlohmann::json json_error(const std::string &msg, int code = -1)
{
    return {{"success", false}, {"error", msg}, {"code", code}};
}
