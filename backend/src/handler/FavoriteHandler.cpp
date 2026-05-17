#include "FavoriteHandler.h"
#include "backend/src/middleware/Auth.h"
#include "backend/src/util/Response.h"
#include "Config.h"

#include <nlohmann/json.hpp>
#include <workflow/MySQLResult.h>

using namespace protocol;

void FavoriteHandler::register_routes(wfrest::HttpServer &server)
{
    // ── POST /api/favorite/add ──
    server.POST("/api/favorite/add", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t file_id = body.value("file_id", (int64_t)0);
        if (file_id <= 0) {
            set_json_response(resp, json_error("file_id is required"));
            return;
        }

        // Verify file belongs to user
        std::string check = "SELECT id FROM tbl_file_v2 WHERE id = "
            + std::to_string(file_id) + " AND uid = " + std::to_string(user.id);

        resp->MySQL(mysql_url(), check, [resp, file_id, &user](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("file not found"));
                return;
            }
            std::vector<MySQLCell> row;
            if (!cursor->fetch_row(row)) {
                set_json_response(resp, json_error("file not found"));
                return;
            }

            std::string sql = "INSERT IGNORE INTO tbl_favorite (uid, file_id) VALUES ("
                + std::to_string(user.id) + ", " + std::to_string(file_id) + ")";

            resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *) {
                set_json_response(resp, json_success());
            });
        });
    }));

    // ── POST /api/favorite/remove ──
    server.POST("/api/favorite/remove", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t file_id = body.value("file_id", (int64_t)0);
        if (file_id <= 0) {
            set_json_response(resp, json_error("file_id is required"));
            return;
        }

        std::string sql = "DELETE FROM tbl_favorite WHERE uid = "
            + std::to_string(user.id) + " AND file_id = " + std::to_string(file_id);

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *) {
            set_json_response(resp, json_success());
        });
    }));

    // ── POST /api/favorite/list ──
    server.POST("/api/favorite/list", require_auth([](
        const wfrest::HttpReq *, wfrest::HttpResp *resp, const User &user)
    {
        // Join with tbl_file_v2 to return full file info (same format as /api/file/list)
        std::string sql =
            "SELECT f.id, f.filename, f.is_folder, f.hashcode, f.size, f.mime_type, "
            "f.created_at, f.updated_at "
            "FROM tbl_favorite fav "
            "JOIN tbl_file_v2 f ON fav.file_id = f.id "
            "WHERE fav.uid = " + std::to_string(user.id) +
            " AND f.tomb = 0 "
            "ORDER BY fav.created_at DESC";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_success({{"files", nlohmann::json::array()}}));
                return;
            }
            nlohmann::json files = nlohmann::json::array();
            std::vector<MySQLCell> row;
            while (cursor->fetch_row(row)) {
                nlohmann::json f;
                f["id"] = row[0].as_ulonglong();
                f["filename"] = row[1].as_string();
                f["is_folder"] = (bool)row[2].as_int();
                f["hashcode"] = row[3].as_string();
                f["size"] = row[4].as_ulonglong();
                f["mime_type"] = row[5].as_string();
                f["created_at"] = row[6].as_datetime();
                f["updated_at"] = row[7].as_datetime();
                files.push_back(f);
            }
            set_json_response(resp, json_success({{"files", files}}));
        });
    }));
}
