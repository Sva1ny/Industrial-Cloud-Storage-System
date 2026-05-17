#include "ShareHandler.h"
#include "backend/src/middleware/Auth.h"
#include "backend/src/util/Response.h"
#include "CryptoUtil.h"
#include "Config.h"

#include <nlohmann/json.hpp>
#include <workflow/MySQLResult.h>

using namespace protocol;

// Generate a random share token
static std::string generate_share_token()
{
    std::string raw = std::to_string(time(nullptr)) + "-"
        + std::to_string(rand());
    return CryptoUtil::generate_hashcode(raw.data(), raw.size());
}

void ShareHandler::register_routes(wfrest::HttpServer &server)
{
    // ── POST /api/share/create ──
    server.POST("/api/share/create", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t file_id = body.value("file_id", (int64_t)0);
        int share_type = body.value("share_type", 0);
        std::string password = body.value("password", "");
        int expire_days = body.value("expire_days", 7);

        if (file_id <= 0) {
            set_json_response(resp, json_error("file_id is required"));
            return;
        }

        std::string token = generate_share_token();
        std::string pwd_hash;
        if (!password.empty())
            pwd_hash = CryptoUtil::hash_password(password, "share");

        std::string sql = "INSERT INTO tbl_share (uid, file_id, share_token, share_type, password_hash, expire_at) "
                          "VALUES (" + std::to_string(user.id) + ", "
                          + std::to_string(file_id) + ", '"
                          + token + "', " + std::to_string(share_type)
                          + ", '" + pwd_hash + "', "
                          + "DATE_ADD(NOW(), INTERVAL " + std::to_string(expire_days) + " DAY))";

        resp->MySQL(mysql_url(), sql, [resp, token](MySQLResultCursor *) {
            set_json_response(resp, json_success({
                {"share_token", token},
                {"share_url", "/api/share/access?token=" + token}
            }));
        });
    }));

    // ── POST /api/share/list ──
    server.POST("/api/share/list", require_auth([](
        const wfrest::HttpReq *, wfrest::HttpResp *resp, const User &user)
    {
        std::string sql = "SELECT s.id, s.share_token, s.share_type, s.download_count, "
                          "s.created_at, s.expire_at, s.is_active, f.filename "
                          "FROM tbl_share s JOIN tbl_file_v2 f ON s.file_id = f.id "
                          "WHERE s.uid = " + std::to_string(user.id) +
                          " ORDER BY s.created_at DESC LIMIT 100";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("no shares"));
                return;
            }
            nlohmann::json shares = nlohmann::json::array();
            std::vector<MySQLCell> row;
            while (cursor->fetch_row(row)) {
                nlohmann::json s;
                s["id"] = row[0].as_ulonglong();
                s["share_token"] = row[1].as_string();
                s["share_type"] = row[2].as_int();
                s["download_count"] = row[3].as_int();
                s["created_at"] = row[4].as_datetime();
                s["expire_at"] = row[5].as_datetime();
                s["is_active"] = (bool)row[6].as_int();
                s["filename"] = row[7].as_string();
                shares.push_back(s);
            }
            set_json_response(resp, json_success({{"shares", shares}}));
        });
    }));

    // ── POST /api/share/delete ──
    server.POST("/api/share/delete", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t share_id = body.value("share_id", (int64_t)0);
        if (share_id <= 0) {
            set_json_response(resp, json_error("share_id is required"));
            return;
        }

        std::string sql = "DELETE FROM tbl_share WHERE id = "
            + std::to_string(share_id) + " AND uid = "
            + std::to_string(user.id);

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *) {
            set_json_response(resp, json_success());
        });
    }));

    // ── GET /api/share/access — public (no auth required) ──
    server.GET("/api/share/access", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp) {
        std::string token = req->query("token");
        if (token.empty()) {
            set_json_response(resp, json_error("token is required"));
            return;
        }

        std::string sql = "SELECT s.file_id, s.share_type, s.password_hash, "
                          "s.expire_at, f.filename, f.is_folder "
                          "FROM tbl_share s JOIN tbl_file_v2 f ON s.file_id = f.id "
                          "WHERE s.share_token = '" + token + "' AND s.is_active = 1";

        resp->MySQL(mysql_url(), sql, [resp, token](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("share not found"));
                return;
            }
            std::vector<MySQLCell> row;
            if (!cursor->fetch_row(row)) {
                set_json_response(resp, json_error("share not found"));
                return;
            }

            // Check expiry
            std::string expire = row[3].as_datetime();
            // Just return file info for now; the web client handles password

            nlohmann::json data;
            data["file_id"] = row[0].as_ulonglong();
            data["share_type"] = row[1].as_int();
            data["has_password"] = !row[2].as_string().empty();
            data["expire_at"] = expire;
            data["filename"] = row[4].as_string();
            data["is_folder"] = (bool)row[5].as_int();

            set_json_response(resp, json_success(data));
        });
    });
}
