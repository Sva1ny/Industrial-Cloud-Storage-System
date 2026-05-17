#include "InvertedIndex.h"
#include "TextExtractor.h"
#include "Util.h"
#include <unordered_map>
#include <algorithm>

std::string InvertedIndex::extract_text(const std::string &filepath)
{
    return TextExtractor::extract(filepath, "");
}

bool InvertedIndex::is_supported(const std::string &filepath)
{
    size_t dot = filepath.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = filepath.substr(dot + 1);
    return TextExtractor::isSupported(ext);
}

IndexData InvertedIndex::prepare_index(const std::string &filepath, int64_t file_id)
{
    IndexData data;

    if (!is_supported(filepath)) return data;

    data.text_content = extract_text(filepath);
    if (data.text_content.empty()) return data;

    // Escape for SQL
    std::string escaped_text = data.text_content;
    // Replace single quotes for SQL
    for (size_t i = 0; i < escaped_text.size(); i++)
        if (escaped_text[i] == '\'') { escaped_text.insert(i, "'"); i++; }

    data.insert_text_sql = "REPLACE INTO tbl_file_text (file_id, text) VALUES ("
        + std::to_string(file_id) + ", '" + mysql_escape(escaped_text) + "')";

    // Delete old index entries
    data.delete_index_sql = "DELETE FROM tbl_inverted_index WHERE file_id = "
        + std::to_string(file_id);

    // Tokenize
    auto tokens = Tokenizer::tokenize(data.text_content);
    if (tokens.empty()) return data;

    // Count term frequency and positions
    std::unordered_map<std::string, std::pair<int, std::vector<int>>> term_map;
    for (const auto &t : tokens) {
        auto &entry = term_map[t.word];
        entry.first++;
        entry.second.push_back(t.position);
    }

    // Build index insert SQLs
    for (const auto &[term, info] : term_map) {
        std::string positions_str;
        for (size_t i = 0; i < info.second.size(); i++) {
            if (i > 0) positions_str += ",";
            positions_str += std::to_string(info.second[i]);
        }
        std::string sql = "INSERT INTO tbl_inverted_index (term, file_id, freq, positions) VALUES ('"
            + mysql_escape(term) + "', "
            + std::to_string(file_id) + ", "
            + std::to_string(info.first) + ", '"
            + positions_str + "')";
        data.insert_index_sqls.push_back(sql);
    }

    return data;
}
