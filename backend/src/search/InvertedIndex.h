#pragma once
#include <string>
#include <vector>
#include <utility>
#include "Tokenizer.h"

struct IndexData {
    std::string text_content;
    std::string insert_text_sql;
    std::string delete_index_sql;
    std::vector<std::string> insert_index_sqls;
};

class InvertedIndex {
public:
    // Index a file: extract text → tokenize → generate SQL
    static IndexData prepare_index(const std::string &filepath, int64_t file_id);

    // Check if file type is supported for indexing
    static bool is_supported(const std::string &filepath);

private:
    // Extract text content from a file (auto-detect type)
    static std::string extract_text(const std::string &filepath);
};
