#pragma once
#include <string>
#include <functional>
#include "User.h"
#include "CryptoUtil.h"
#include "wfrest/HttpServer.h"
#include "backend/src/util/Response.h"

inline std::string extract_token(const wfrest::HttpReq *req)
{
    // Try Authorization header first
    if (req->has_header("Authorization")) {
        const std::string &auth = req->header("Authorization");
        if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ") {
            return auth.substr(7);
        }
    }
    // Fallback to query param (legacy)
    return req->query("token");
}

inline bool extract_user(const wfrest::HttpReq *req, User &user)
{
    std::string token = extract_token(req);
    if (token.empty()) return false;
    return CryptoUtil::verify_token(token, user);
}

// require_auth wraps a handler that takes (req, resp, user)
inline std::function<void(const wfrest::HttpReq *, wfrest::HttpResp *)>
require_auth(std::function<void(const wfrest::HttpReq *, wfrest::HttpResp *, const User &)> handler)
{
    return [handler](const wfrest::HttpReq *req, wfrest::HttpResp *resp) {
        User user;
        if (!extract_user(req, user)) {
            resp->set_status(HttpStatusUnauthorized);
            set_json_response(resp, json_error("unauthorized"));
            return;
        }
        handler(req, resp, user);
    };
}
