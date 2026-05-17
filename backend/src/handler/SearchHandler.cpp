#include "SearchHandler.h"
#include "backend/src/middleware/Auth.h"
#include "backend/src/util/Response.h"
#include "backend/src/search/Tokenizer.h"
#include "Config.h"
#include "Util.h"

#include <nlohmann/json.hpp>
#include <workflow/MySQLResult.h>
#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <algorithm>
#include <set>

using namespace protocol;

// Remove or replace bytes that form invalid UTF-8 sequences (for JSON serialization)
static std::string sanitize_utf8(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c <= 0x7F) {
            out += c;                       // ASCII
        } else if (c >= 0xC2 && c <= 0xDF && i + 1 < s.size() && (unsigned char)s[i+1] >= 0x80 && (unsigned char)s[i+1] <= 0xBF) {
            out += s[i]; out += s[++i];     // 2-byte sequence
        } else if (c >= 0xE0 && c <= 0xEF && i + 2 < s.size() && (unsigned char)s[i+1] >= 0x80 && (unsigned char)s[i+1] <= 0xBF && (unsigned char)s[i+2] >= 0x80 && (unsigned char)s[i+2] <= 0xBF) {
            // 3-byte sequence (includes CJK)
            unsigned char b1 = (unsigned char)s[i+1];
            unsigned char b2 = (unsigned char)s[i+2];
            // Reject overlong and surrogates
            if (!(c == 0xE0 && b1 < 0xA0) && !(c == 0xED && b1 > 0x9F)) {
                out += s[i]; out += s[i+1]; out += s[i+2]; i += 2;
            }
        } else {
            out += ' ';  // Replace invalid byte with space
        }
    }
    return out;
}

void SearchHandler::register_routes(wfrest::HttpServer &server)
{
    // POST /api/search/fulltext — full-text search
    server.POST("/api/search/fulltext", require_auth([](
        const wfrest::HttpReq *req, wfrest::HttpResp *resp, const User &user)
    {
        nlohmann::json body;
        try { body = nlohmann::json::parse(req->body()); } catch (...) {}

        std::string query = body.value("query", "");
        if (query.empty()) {
            set_json_response(resp, json_error("query is required"));
            return;
        }

        int limit = std::min(body.value("limit", 20), 100);
        int offset = std::max(body.value("offset", 0), 0);

        // Tokenize the query
        auto tokens = Tokenizer::tokenize(query);
        if (tokens.empty()) {
            set_json_response(resp, json_success({{"total", 0}, {"results", nlohmann::json::array()}}));
            return;
        }

        // Remove duplicate terms for the WHERE clause
        std::set<std::string> unique_terms;
        for (const auto &t : tokens)
            unique_terms.insert(t.word);

        // Build IN clause
        std::string term_list;
        for (const auto &t : unique_terms) {
            if (!term_list.empty()) term_list += ",";
            term_list += "'" + mysql_escape(t) + "'";
        }

        // Count total documents for this user (for TF-IDF)
        std::string count_sql = "SELECT COUNT(*) FROM tbl_file_v2 WHERE uid = "
            + std::to_string(user.id) + " AND tomb = 0 AND is_folder = 0";

        resp->MySQL(mysql_url(), count_sql, [resp, user, term_list, limit, offset, query](MySQLResultCursor *cursor) {
            int total_docs = 100;
            int s = cursor->get_cursor_status();
            if (s == MYSQL_STATUS_GET_RESULT || s == MYSQL_STATUS_END) {
                std::vector<MySQLCell> row;
                if (cursor->fetch_row(row)) {
                    std::string cnt_str = row[0].as_string();
                    if (!cnt_str.empty())
                        total_docs = std::atoi(cnt_str.c_str());
                }
            }
            if (total_docs < 1) total_docs = 1;

            // Search query: join inverted index with file metadata
            // Score = sum of tf * idf across all matching terms
            std::string search_sql =
                "SELECT idx.file_id, idx.term, idx.freq, "
                "  (SELECT COUNT(*) FROM tbl_inverted_index WHERE term = idx.term) AS df "
                "FROM tbl_inverted_index idx "
                "JOIN tbl_file_v2 f ON idx.file_id = f.id "
                "WHERE idx.term IN (" + term_list + ") "
                "  AND f.uid = " + std::to_string(user.id) + " "
                "  AND f.tomb = 0 AND f.is_folder = 0 "
                "ORDER BY idx.file_id";

            resp->MySQL(mysql_url(), search_sql, [resp, user, total_docs, term_list, limit, offset, query](MySQLResultCursor *cursor) {
                int s = cursor->get_cursor_status();
                if (s != MYSQL_STATUS_GET_RESULT && s != MYSQL_STATUS_END) {
                    set_json_response(resp, json_success({{"total", 0}, {"results", nlohmann::json::array()}}));
                    return;
                }

                // Aggregate: file_id → { term → (freq, df) }
                std::unordered_map<int64_t, std::unordered_map<std::string, std::pair<int, int>>> file_scores;
                std::vector<MySQLCell> row;
                while (cursor->fetch_row(row)) {
                    int64_t file_id = row[0].as_ulonglong();
                    std::string term = row[1].as_string();
                    int freq = row[2].as_int();
                    int df = std::max(row[3].as_int(), 1);
                    file_scores[file_id][term] = {freq, df};
                }

                if (file_scores.empty()) {
                    set_json_response(resp, json_success({{"total", 0}, {"results", nlohmann::json::array()}}));
                    return;
                }

                // Get file details for matching files
                std::string file_ids_str;
                for (const auto &[fid, _] : file_scores) {
                    if (!file_ids_str.empty()) file_ids_str += ",";
                    file_ids_str += std::to_string(fid);
                }

                std::string detail_sql = "SELECT v.id, v.filename, v.size, v.updated_at, v.hashcode, "
                                          "LEFT(COALESCE(t.text, ''), 500) AS file_text "
                                          "FROM tbl_file_v2 v "
                                          "LEFT JOIN tbl_file_text t ON t.file_id = v.id "
                                          "WHERE v.id IN (" + file_ids_str + ")";

                resp->MySQL(mysql_url(), detail_sql, [resp, file_scores, total_docs, term_list, limit, offset, query](MySQLResultCursor *cursor) {
                    int s = cursor->get_cursor_status();
                    if (s != MYSQL_STATUS_GET_RESULT && s != MYSQL_STATUS_END) {
                        set_json_response(resp, json_success({{"total", 0}, {"results", nlohmann::json::array()}}));
                        return;
                    }

                    // Build result list with scores
                    struct Result {
                        int64_t file_id;
                        std::string filename;
                        int64_t size;
                        std::string updated_at;
                        std::string hashcode;
                        double score;
                        std::string snippet;
                    };
                    std::vector<Result> results;

                    std::vector<MySQLCell> row;
                    while (cursor->fetch_row(row)) {
                        int64_t file_id = row[0].as_ulonglong();
                        Result r;
                        r.file_id = file_id;
                        r.filename = row[1].as_string();
                        r.size = row[2].as_ulonglong();
                        r.updated_at = row[3].as_datetime();
                        r.hashcode = row[4].as_string();
                        std::string file_text = row[5].as_string();

                        // Compute TF-IDF score
                        double score = 0;
                        auto it = file_scores.find(file_id);
                        if (it != file_scores.end()) {
                            for (const auto &[term_data, freq_df] : it->second) {
                                int tf = freq_df.first;
                                int df = freq_df.second;
                                #ifdef DEBUG
                                std::cerr << "[SEARCH] term=" << term_data
                                    << " tf=" << tf << " df=" << df
                                    << " N=" << total_docs << std::endl;
                                #endif
                                double tf_weight = 1.0 + (tf > 1 ? std::log(double(tf)) : 0);
                                double idf = std::log(double(total_docs + 1) / double(df + 1));
                                score += tf_weight * idf;
                            }
                        }
                        // Give a base score for any match
                        if (score == 0 && it != file_scores.end() && !it->second.empty()) score = 0.5;
                        r.score = score;

                        // Generate snippet
                        if (!file_text.empty()) {
                            // Sanitize text to ensure valid UTF-8 for JSON
                            std::string clean = sanitize_utf8(file_text);
                            // Find earliest occurrence of the query in text
                            size_t best_pos = clean.find(query);
                            const int CONTEXT = 60;
                            if (best_pos != std::string::npos) {
                                size_t start = (best_pos > CONTEXT) ? best_pos - CONTEXT : 0;
                                size_t end = std::min(best_pos + CONTEXT, clean.size());
                                std::string snip = clean.substr(start, end - start);
                                if (start > 0) snip = "..." + snip;
                                if (end < clean.size()) snip += "...";
                                // Replace newlines
                                for (char &c : snip) if (c == '\n' || c == '\r') c = ' ';
                                // Sanitize again in case substr cut mid-char
                                r.snippet = sanitize_utf8(snip);
                            } else if (clean.size() > 80) {
                                r.snippet = sanitize_utf8(clean.substr(0, 80)) + "...";
                            } else {
                                r.snippet = clean;
                            }
                        }

                        results.push_back(r);
                    }

                    // Sort by score descending
                    std::sort(results.begin(), results.end(), [](const Result &a, const Result &b) {
                        return a.score > b.score;
                    });

                    // Apply pagination
                    int total = results.size();
                    if (offset >= total) {
                        set_json_response(resp, json_success({{"total", total}, {"results", nlohmann::json::array()}}));
                        return;
                    }
                    int end = std::min(offset + limit, total);

                    // Build response
                    nlohmann::json json_results = nlohmann::json::array();
                    for (int i = offset; i < end; i++) {
                        const auto &r = results[i];
                        nlohmann::json jr;
                        jr["file_id"] = r.file_id;
                        jr["filename"] = r.filename;
                        jr["size"] = r.size;
                        jr["updated_at"] = r.updated_at;
                        jr["score"] = std::round(r.score * 100) / 100.0;
                        jr["snippet"] = r.snippet;
                        json_results.push_back(jr);
                    }

                    set_json_response(resp, json_success({
                        {"total", total},
                        {"results", json_results}
                    }));
                });
            });
        });
    }));
}
