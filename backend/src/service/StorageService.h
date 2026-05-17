#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "CryptoUtil.h"

// Content-addressable storage: files/data/<hash[0:2]>/<hash[2:4]>/<hash>
class StorageService {
public:
    // Save content to content-addressable path. Returns the storage path.
    // If content already exists (same hash), returns existing path without writing.
    static std::string save_file(const std::string &content, const std::string &hashcode)
    {
        std::string path = build_path(hashcode);
        // Check if already exists
        if (access(path.c_str(), F_OK) == 0)
            return path;

        // Ensure directory exists
        std::string dir = "files/data/" + hashcode.substr(0, 2) + "/" + hashcode.substr(2, 2);
        if (access(dir.c_str(), F_OK) != 0)
            mkdir(dir.c_str(), 0755);

        // Write file
        int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) return "";
        ::write(fd, content.data(), content.size());
        ::close(fd);
        return path;
    }

    static std::string read_file(const std::string &storage_path)
    {
        std::ifstream in(storage_path, std::ios::binary | std::ios::ate);
        if (!in) return "";
        size_t size = in.tellg();
        in.seekg(0);
        std::string content(size, '\0');
        in.read(&content[0], size);
        return content;
    }

    static bool delete_file(const std::string &storage_path)
    {
        return ::unlink(storage_path.c_str()) == 0;
    }

    static bool file_exists(const std::string &storage_path)
    {
        return access(storage_path.c_str(), F_OK) == 0;
    }

    static std::string compute_hash(const std::string &content)
    {
        return CryptoUtil::generate_hashcode(content.data(), content.size());
    }

    static std::string build_path(const std::string &hashcode)
    {
        return "files/data/" + hashcode.substr(0, 2) + "/"
               + hashcode.substr(2, 2) + "/" + hashcode;
    }

    // Ensure per-user directory exists (legacy compat)
    static std::string ensure_user_dir(const std::string &username)
    {
        std::string dir = "files/" + username;
        if (access(dir.c_str(), F_OK) != 0)
            mkdir(dir.c_str(), 0777);
        return dir;
    }
};
