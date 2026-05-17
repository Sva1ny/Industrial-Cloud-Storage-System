#include "AuthHandler.h"
#include "backend/src/middleware/Auth.h"
#include "backend/src/util/Response.h"
#include "CryptoUtil.h"
#include "Config.h"

#include <nlohmann/json.hpp>

void AuthHandler::register_routes(wfrest::HttpServer &server)
{
    // POST /api/auth/refresh — refresh JWT token
    server.POST("/api/auth/refresh", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp) {
        User user;
        if (!extract_user(req, user)) {
            resp->set_status(HttpStatusUnauthorized);
            set_json_response(resp, json_error("unauthorized"));
            return;
        }
        std::string new_token = CryptoUtil::generate_token(user);
        set_json_response(resp, json_success({{"token", new_token}}));
    });
}
