#pragma once
#include <string>

struct FileRecord {
    int64_t id = 0;
    int uid = 0;
    int64_t parentId = 0;
    std::string filename;
    bool isFolder = false;
    std::string hashcode;
    int64_t size = 0;
    std::string mimeType;
    std::string storagePath;  // actual disk location
    std::string createdAt;
    std::string updatedAt;
    bool tomb = false;
    std::string deletedAt;
};
