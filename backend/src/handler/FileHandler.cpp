#include "FileHandler.h"
#include "backend/src/middleware/Auth.h"
#include "backend/src/util/Response.h"
#include "backend/src/service/FileService.h"
#include "backend/src/service/StorageService.h"
#include "backend/src/model/FileRecord.h"
#include "backend/src/search/InvertedIndex.h"
#include "Config.h"
#include "Util.h"

#include <nlohmann/json.hpp>
#include <workflow/MySQLResult.h>
#include <thread>
#include <chrono>

using namespace protocol;

void FileHandler::register_routes(wfrest::HttpServer &server)
{
    // ── POST /api/file/list ──
    server.POST("/api/file/list", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t parent_id = body.value("parent_id", (int64_t)0);
        std::string sort_by = FileService::safe_sort_column(body.value("sort_by", "filename"));
        std::string sort_order = FileService::safe_sort_order(body.value("sort_order", "asc"));
        int limit = std::min(body.value("limit", 50), 200);
        int offset = std::max(body.value("offset", 0), 0);

        std::string sql = "SELECT id, filename, is_folder, hashcode, size, mime_type, "
                          "created_at, updated_at FROM tbl_file_v2 "
                          "WHERE uid = " + std::to_string(user.id) +
                          " AND parent_id = " + std::to_string(parent_id) +
                          " AND tomb = 0 "
                          "ORDER BY is_folder DESC, " + sort_by + " " + sort_order +
                          " LIMIT " + std::to_string(limit) +
                          " OFFSET " + std::to_string(offset);

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("database error"));
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

    // ── POST /api/file/create_folder ──
    server.POST("/api/file/create_folder", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t parent_id = body.value("parent_id", (int64_t)0);
        std::string filename = mysql_escape(body.value("filename", ""));
        if (filename.empty()) {
            set_json_response(resp, json_error("filename is required"));
            return;
        }

        std::string check_sql = "SELECT id FROM tbl_file_v2 WHERE uid = "
            + std::to_string(user.id) + " AND parent_id = "
            + std::to_string(parent_id) + " AND filename = '" + filename
            + "' AND tomb = 0 AND is_folder = 1 LIMIT 1";

        resp->MySQL(mysql_url(), check_sql, [resp, parent_id, filename, uid = user.id](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() == MYSQL_STATUS_GET_RESULT) {
                std::vector<MySQLCell> row;
                if (cursor->fetch_row(row)) {
                    set_json_response(resp, json_error("folder already exists"));
                    return;
                }
            }
            std::string insert_sql = "INSERT INTO tbl_file_v2 (uid, parent_id, filename, is_folder) VALUES ("
                + std::to_string(uid) + ", " + std::to_string(parent_id)
                + ", '" + filename + "', 1)";

            resp->MySQL(mysql_url(), insert_sql, [resp](MySQLResultCursor *) {
                set_json_response(resp, json_success());
            });
        });
    }));

    // ── POST /api/file/rename ──
    server.POST("/api/file/rename", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t file_id = body.value("file_id", (int64_t)0);
        std::string new_name = mysql_escape(body.value("new_filename", ""));
        if (file_id <= 0 || new_name.empty()) {
            set_json_response(resp, json_error("file_id and new_filename are required"));
            return;
        }

        std::string sql = "UPDATE tbl_file_v2 SET filename = '" + new_name
            + "' WHERE id = " + std::to_string(file_id)
            + " AND uid = " + std::to_string(user.id) + " AND tomb = 0";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *cursor) {
            bool ok = cursor->get_cursor_status() == MYSQL_STATUS_OK
                   || cursor->get_cursor_status() == MYSQL_STATUS_END;
            if (ok)
                set_json_response(resp, json_success());
            else
                set_json_response(resp, json_error("file not found or rename failed"));
        });
    }));

    // ── POST /api/file/move ──
    server.POST("/api/file/move", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t target_parent_id = body.value("target_parent_id", (int64_t)0);
        auto file_ids = body["file_ids"];
        if (!file_ids.is_array() || file_ids.empty()) {
            set_json_response(resp, json_error("file_ids is required"));
            return;
        }

        std::string ids;
        for (size_t i = 0; i < file_ids.size(); i++)
            ids += (i ? "," : "") + std::to_string(file_ids[i].get<int64_t>());

        std::string sql = "UPDATE tbl_file_v2 SET parent_id = "
            + std::to_string(target_parent_id) + " WHERE id IN (" + ids
            + ") AND uid = " + std::to_string(user.id) + " AND tomb = 0";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *) {
            set_json_response(resp, json_success());
        });
    }));

    // ── POST /api/file/copy ──
    server.POST("/api/file/copy", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t target_parent_id = body.value("target_parent_id", (int64_t)0);
        auto file_ids = body["file_ids"];
        if (!file_ids.is_array() || file_ids.empty()) {
            set_json_response(resp, json_error("file_ids is required"));
            return;
        }

        std::string ids;
        for (size_t i = 0; i < file_ids.size(); i++)
            ids += (i ? "," : "") + std::to_string(file_ids[i].get<int64_t>());

        std::string sql = "INSERT INTO tbl_file_v2 (uid, parent_id, filename, is_folder, hashcode, size, mime_type, storage_path) "
                          "SELECT uid, " + std::to_string(target_parent_id)
                          + ", filename, is_folder, hashcode, size, mime_type, storage_path "
                          "FROM tbl_file_v2 WHERE id IN (" + ids
                          + ") AND uid = " + std::to_string(user.id) + " AND tomb = 0";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *) {
            set_json_response(resp, json_success());
        });
    }));

    // ── POST /api/file/batch_delete ──
    server.POST("/api/file/batch_delete", require_auth([](
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

        std::string sql = "UPDATE tbl_file_v2 SET tomb = 1, deleted_at = NOW() "
                          "WHERE id IN (" + ids + ") AND uid = "
                          + std::to_string(user.id) + " AND tomb = 0";

        resp->MySQL(mysql_url(), sql, [resp, ids, uid = user.id](MySQLResultCursor *) {
            std::string trash_sql = "INSERT INTO tbl_trash (file_id, uid, original_parent_id, original_filename, expire_at) "
                                    "SELECT id, uid, parent_id, filename, DATE_ADD(NOW(), INTERVAL 30 DAY) "
                                    "FROM tbl_file_v2 WHERE id IN (" + ids + ") AND uid = "
                                    + std::to_string(uid);

            resp->MySQL(mysql_url(), trash_sql, [resp](MySQLResultCursor *) {
                set_json_response(resp, json_success());
            });
        });
    }));

    // ── POST /api/file/search ──
    server.POST("/api/file/search", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        std::string query = mysql_escape(body.value("query", ""));
        if (query.empty()) {
            set_json_response(resp, json_error("query is required"));
            return;
        }

        std::string sql = "SELECT id, filename, is_folder, hashcode, size, mime_type, "
                          "created_at, updated_at FROM tbl_file_v2 "
                          "WHERE uid = " + std::to_string(user.id) +
                          " AND tomb = 0 AND filename LIKE '%" + query + "%' "
                          "ORDER BY is_folder DESC, filename ASC LIMIT 100";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("search failed"));
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

    // ── POST /api/file/detail ──
    server.POST("/api/file/detail", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t file_id = body.value("file_id", (int64_t)0);
        if (file_id <= 0) {
            set_json_response(resp, json_error("file_id is required"));
            return;
        }

        std::string sql = "SELECT id, filename, is_folder, hashcode, size, mime_type, "
                          "created_at, updated_at, parent_id FROM tbl_file_v2 "
                          "WHERE id = " + std::to_string(file_id) +
                          " AND uid = " + std::to_string(user.id) + " AND tomb = 0";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("not found"));
                return;
            }
            std::vector<MySQLCell> row;
            if (!cursor->fetch_row(row)) {
                set_json_response(resp, json_error("not found"));
                return;
            }
            nlohmann::json f;
            f["id"] = row[0].as_ulonglong();
            f["filename"] = row[1].as_string();
            f["is_folder"] = (bool)row[2].as_int();
            f["hashcode"] = row[3].as_string();
            f["size"] = row[4].as_ulonglong();
            f["mime_type"] = row[5].as_string();
            f["created_at"] = row[6].as_datetime();
            f["updated_at"] = row[7].as_datetime();
            f["parent_id"] = row[8].as_ulonglong();
            set_json_response(resp, json_success({{"file", f}}));
        });
    }));

    // ── POST /api/file/download — download by file_id ──
    server.POST("/api/file/download", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t file_id = body.value("file_id", (int64_t)0);
        if (file_id <= 0) {
            set_json_response(resp, json_error("file_id is required"));
            return;
        }

        std::string sql = "SELECT filename, storage_path, hashcode FROM tbl_file_v2 "
                          "WHERE id = " + std::to_string(file_id) +
                          " AND uid = " + std::to_string(user.id) +
                          " AND tomb = 0 AND is_folder = 0";

        resp->MySQL(mysql_url(), sql, [resp](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("not found"));
                return;
            }
            std::vector<MySQLCell> row;
            if (!cursor->fetch_row(row)) {
                set_json_response(resp, json_error("not found"));
                return;
            }
            std::string filename = row[0].as_string();
            std::string storage_path = row[1].as_string();
            std::string hashcode = row[2].as_string();

            // Support both content-addressed and legacy paths
            std::string resolved;
            if (!storage_path.empty() && StorageService::file_exists(storage_path)) {
                resolved = storage_path;
            } else if (!hashcode.empty()) {
                resolved = StorageService::build_path(hashcode);
                if (!StorageService::file_exists(resolved)) resolved = "";
            }
            if (resolved.empty()) {
                set_json_response(resp, json_error("file not found on disk"));
                return;
            }
            resp->set_header_pair("Content-Disposition",
                "attachment; filename=" + filename);
            resp->File(resolved);
        });
    }));

    // ── POST /api/file/download_zip — download folder as ZIP ──
    server.POST("/api/file/download_zip", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t folder_id = body.value("folder_id", (int64_t)0);
        if (folder_id <= 0) {
            set_json_response(resp, json_error("folder_id is required"));
            return;
        }

        // Get folder name
        std::string name_sql = "SELECT filename FROM tbl_file_v2 WHERE id = "
            + std::to_string(folder_id) + " AND uid = "
            + std::to_string(user.id) + " AND is_folder = 1 AND tomb = 0";

        resp->MySQL(mysql_url(), name_sql, [resp, folder_id, uid = user.id](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("folder not found"));
                return;
            }
            std::vector<MySQLCell> row;
            if (!cursor->fetch_row(row)) {
                set_json_response(resp, json_error("folder not found"));
                return;
            }
            std::string folder_name = row[0].as_string();

            // Collect all files in this folder (non-recursive for simplicity)
            std::string file_sql = "SELECT filename, storage_path, hashcode FROM tbl_file_v2 "
                                   "WHERE parent_id = " + std::to_string(folder_id) +
                                   " AND uid = " + std::to_string(uid) +
                                   " AND tomb = 0 AND is_folder = 0";

            resp->MySQL(mysql_url(), file_sql, [resp, folder_name](MySQLResultCursor *cursor) {
                if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                    set_json_response(resp, json_error("no files in folder"));
                    return;
                }

                // Create temp ZIP
                std::string tmpdir = "/tmp/clouddisk_zip_" + std::to_string(time(nullptr));
                std::string cmd = "mkdir -p " + tmpdir + " && ";

                std::vector<MySQLCell> row;
                bool has_files = false;
                while (cursor->fetch_row(row)) {
                    std::string fname = row[0].as_string();
                    std::string storage_path = row[1].as_string();
                    std::string hashcode = row[2].as_string();

                    std::string resolved;
                    if (!storage_path.empty() && StorageService::file_exists(storage_path))
                        resolved = storage_path;
                    else if (!hashcode.empty()) {
                        resolved = StorageService::build_path(hashcode);
                        if (!StorageService::file_exists(resolved)) resolved = "";
                    }
                    if (!resolved.empty()) {
                        cmd += "cp \"" + resolved + "\" \"" + tmpdir + "/" + fname + "\" && ";
                        has_files = true;
                    }
                }

                if (!has_files) {
                    set_json_response(resp, json_error("no downloadable files in folder"));
                    system(("rm -rf " + tmpdir).c_str());
                    return;
                }

                std::string zip_path = tmpdir + ".zip";
                cmd += "cd \"" + tmpdir + "\" && zip -r \"" + zip_path + "\" . >/dev/null 2>&1";

                int ret = system(cmd.c_str());
                if (ret != 0) {
                    system(("rm -rf " + tmpdir + " " + zip_path).c_str());
                    set_json_response(resp, json_error("failed to create zip"));
                    return;
                }

                // Stream the ZIP
                resp->set_header_pair("Content-Disposition",
                    "attachment; filename=" + folder_name + ".zip");
                resp->File(zip_path);

                // Cleanup after file is served (async)
                // Note: wfrest serves files asynchronously, so we schedule cleanup
                // For simplicity, cleanup in a separate task
                std::thread([tmpdir, zip_path]() {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    system(("rm -rf " + tmpdir + " " + zip_path).c_str());
                }).detach();
            });
        });
    }));

    // ── POST /api/file/history — list version history ──
    server.POST("/api/file/history", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t file_id = body.value("file_id", (int64_t)0);
        if (file_id <= 0) {
            set_json_response(resp, json_error("file_id is required"));
            return;
        }

        // Check file belongs to user
        std::string check_sql = "SELECT filename, hashcode, size, storage_path, updated_at "
                                "FROM tbl_file_v2 WHERE id = " + std::to_string(file_id) +
                                " AND uid = " + std::to_string(user.id) + " AND tomb = 0";

        resp->MySQL(mysql_url(), check_sql, [resp, file_id, uid = user.id](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("file not found"));
                return;
            }
            std::vector<MySQLCell> row;
            if (!cursor->fetch_row(row)) {
                set_json_response(resp, json_error("file not found"));
                return;
            }

            // Current version info
            nlohmann::json versions = nlohmann::json::array();
            nlohmann::json current;
            current["version"] = 0;
            current["hashcode"] = row[1].as_string();
            current["size"] = row[2].as_ulonglong();
            current["storage_path"] = row[3].as_string();
            current["created_at"] = row[4].as_datetime();
            current["is_current"] = true;
            versions.push_back(current);

            // Get history versions
            std::string history_sql = "SELECT id, version, hashcode, size, storage_path, created_at "
                                      "FROM tbl_file_history WHERE file_id = "
                                      + std::to_string(file_id) + " ORDER BY version DESC";

            resp->MySQL(mysql_url(), history_sql, [resp, versions](MySQLResultCursor *cursor) mutable {
                if (cursor->get_cursor_status() == MYSQL_STATUS_GET_RESULT) {
                    std::vector<MySQLCell> row;
                    while (cursor->fetch_row(row)) {
                        nlohmann::json v;
                        v["history_id"] = row[0].as_ulonglong();
                        v["version"] = row[1].as_int();
                        v["hashcode"] = row[2].as_string();
                        v["size"] = row[3].as_ulonglong();
                        v["storage_path"] = row[4].as_string();
                        v["created_at"] = row[5].as_datetime();
                        v["is_current"] = false;
                        versions.push_back(v);
                    }
                }
                set_json_response(resp, json_success({{"versions", versions}}));
            });
        });
    }));

    // ── POST /api/file/reindex — re-index a file for fulltext search ──
    server.POST("/api/file/reindex", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        int64_t file_id = body.value("file_id", (int64_t)0);
        if (file_id <= 0) {
            set_json_response(resp, json_error("file_id is required"));
            return;
        }

        // Look up file storage path
        std::string sql = "SELECT storage_path FROM tbl_file_v2 "
                          "WHERE id = " + std::to_string(file_id) +
                          " AND uid = " + std::to_string(user.id) +
                          " AND tomb = 0 AND is_folder = 0";

        resp->MySQL(mysql_url(), sql, [resp, file_id](MySQLResultCursor *cursor) {
            if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                set_json_response(resp, json_error("file not found"));
                return;
            }
            std::vector<MySQLCell> row;
            if (!cursor->fetch_row(row)) {
                set_json_response(resp, json_error("file not found"));
                return;
            }
            std::string storage_path = row[0].as_string();

            // Extract text, tokenize, and generate index SQL
            auto index_data = InvertedIndex::prepare_index(storage_path, file_id);
            if (index_data.insert_text_sql.empty()) {
                set_json_response(resp, json_error("unsupported file type or empty content"));
                return;
            }

            // Chain: delete old indices → insert new text → insert new index entries
            resp->MySQL(mysql_url(), index_data.delete_index_sql, [resp, index_data](MySQLResultCursor *) {
                resp->MySQL(mysql_url(), index_data.insert_text_sql, [resp, index_data](MySQLResultCursor *) {
                    if (index_data.insert_index_sqls.empty()) {
                        set_json_response(resp, json_success({{"terms", 0}}));
                        return;
                    }

                    // Build multi-row INSERT
                    std::string batch_sql = "INSERT INTO tbl_inverted_index (term, file_id, freq, positions) VALUES ";
                    for (size_t i = 0; i < index_data.insert_index_sqls.size(); i++) {
                        auto val_pos = index_data.insert_index_sqls[i].find("VALUES");
                        if (val_pos != std::string::npos) {
                            if (i > 0) batch_sql += ",";
                            batch_sql += index_data.insert_index_sqls[i].substr(val_pos + 6);
                        }
                    }

                    resp->MySQL(mysql_url(), batch_sql, [resp, index_data](MySQLResultCursor *) {
                        set_json_response(resp, json_success({{"terms", (int)index_data.insert_index_sqls.size()}}));
                    });
                });
            });
        });
    }));
}
