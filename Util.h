#pragma once
#include <string>

// MySQL 字符串转义，防止 SQL 注入
inline std::string mysql_escape(const std::string& s)
{
    std::string result;
    result.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\'': result += "\\'"; break;
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\0': result += "\\0"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\x1a': result += "\\Z"; break;
            default: result += c;
        }
    }
    return result;
}
