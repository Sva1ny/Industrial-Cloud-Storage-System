#include "TrashHandler.h"
#include "backend/src/middleware/Auth.h"
#include "backend/src/util/Response.h"
#include "Config.h"

#include <nlohmann/json.hpp>
#include <workflow/MySQLResult.h>

using namespace protocol;

void TrashHandler::register_routes(wfrest::HttpServer &server)
{
    // ── POST /api/trash/list ──
    server.POST("/api/trash/list", require_auth([](
        const wfrest::HttpReq *, wfrest::HttpResp *resp, const User &user)
    {
        std::string sql = "SELECT f.id, f.filename, f.is_folder, f.size, f.deleted_at, "
                          "t.original_parent_id, t.original_filename, t.expire_at "
                          "FROM tbl_file_v2 f "
                          "JOIN tbl_trash t ON f.id = t.file_id "
                          "WHERE f.uid = " + std::to_string(user.id) +
                          " AND f.tomb = 1 "
                          "ORDER BY f.deleted_at DESC LIMIT 200";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("no trash data"));
                return;
            }
            nlohmann::json items = nlohmann::json::array();
            std::vector<MySQLCell> row;
            while (cursor->fetch_row(row)) {
                nlohmann::json item;
                item["id"] = row[0].as_ulonglong();
                item["filename"] = row[1].as_string();
                item["is_folder"] = (bool)row[2].as_int();
                item["size"] = row[3].as_ulonglong();
                item["deleted_at"] = row[4].as_datetime();
                item["original_parent_id"] = row[5].as_ulonglong();
                item["original_filename"] = row[6].as_string();
                item["expire_at"] = row[7].as_datetime();
                items.push_back(item);
            }
            set_json_response(resp, json_success({{"items", items}}));
        });
    }));

    // ── POST /api/trash/restore ──
    server.POST("/api/trash/restore", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        auto file_ids = body["file_ids"];
        if (!file_ids.is_array() || file_ids.empty()) {
            set_json_response(resp, json_error("file_ids is required"));
            return;
        }

        std::string ids;
        for (size_t i = 0; i < file_ids.size(); i++)
            ids += (i ? "," : "") + std::to_string(file_ids[i].get<int64_t>());

        // Restore: set tomb=0, clear deleted_at
        std::string sql = "UPDATE tbl_file_v2 SET tomb = 0, deleted_at = NULL "
                          "WHERE id IN (" + ids + ") AND uid = "
                          + std::to_string(user.id);

        resp->MySQL(mysql_url(), sql, [resp, ids, uid = user.id](MySQLResultCursor *) {
            // Remove from trash records
            std::string clean = "DELETE FROM tbl_trash WHERE file_id IN ("
                + ids + ") AND uid = " + std::to_string(uid);

            resp->MySQL(mysql_url(), clean, [resp](MySQLResultCursor *) {
                set_json_response(resp, json_success());
            });
        });
    }));

    // ── POST /api/trash/empty ──
    server.POST("/api/trash/empty", require_auth([](
        const wfrest::HttpReq *, wfrest::HttpResp *resp, const User &user)
    {
        // Permanently delete all trashed files for this user
        std::string sql = "DELETE t FROM tbl_trash t "
                          "JOIN tbl_file_v2 f ON t.file_id = f.id "
                          "WHERE f.uid = " + std::to_string(user.id) + " AND f.tomb = 1";

        resp->MySQL(mysql_url(), sql, [resp, uid = user.id](MySQLResultCursor *) {
            // Delete the actual file records
            std::string del = "DELETE FROM tbl_file_v2 WHERE uid = "
                + std::to_string(uid) + " AND tomb = 1";

            resp->MySQL(mysql_url(), del, [resp](MySQLResultCursor *) {
                set_json_response(resp, json_success());
            });
        });
    }));

    // ── POST /api/trash/delete_forever ──
    server.POST("/api/trash/delete_forever", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        auto file_ids = body["file_ids"];
        if (!file_ids.is_array() || file_ids.empty()) {
            set_json_response(resp, json_error("file_ids is required"));
            return;
        }

        std::string ids;
        for (size_t i = 0; i < file_ids.size(); i++)
            ids += (i ? "," : "") + std::to_string(file_ids[i].get<int64_t>());

        // Delete trash records first
        std::string sql = "DELETE FROM tbl_trash WHERE file_id IN ("
            + ids + ") AND uid = " + std::to_string(user.id);

        resp->MySQL(mysql_url(), sql, [resp, ids, uid = user.id](MySQLResultCursor *) {
            std::string del = "DELETE FROM tbl_file_v2 WHERE id IN ("
                + ids + ") AND uid = " + std::to_string(uid);

            resp->MySQL(mysql_url(), del, [resp](MySQLResultCursor *) {
                set_json_response(resp, json_success());
            });
        });
    }));
}
