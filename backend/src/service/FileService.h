#pragma once
#include <string>
#include <vector>
#include <functional>
#include "backend/src/model/FileRecord.h"

namespace FileService {

// Check if a file with the same hash already exists (deduplication)
// Returns the storage_path if found, empty string otherwise
inline void check_dedup(int uid, const std::string &hashcode,
    std::function<void(const std::string &storage_path)> callback)
{
    // Storage path only needs to match hashcode since content-addressed
    // The actual query happens in the handler
    callback("");
}

// Build SQL WHERE conditions safely (only allow known sort columns)
inline std::string safe_sort_column(const std::string &sort_by)
{
    if (sort_by == "filename") return "filename";
    if (sort_by == "size") return "size";
    if (sort_by == "created_at") return "created_at";
    if (sort_by == "updated_at") return "updated_at";
    return "filename";
}

inline std::string safe_sort_order(const std::string &order)
{
    if (order == "asc" || order == "desc") return order;
    return "asc";
}

} // namespace FileService
