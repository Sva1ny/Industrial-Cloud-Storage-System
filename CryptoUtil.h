#pragma once
#include <string>

#include "User.h"
class CryptoUtil
{
public:
    static std::string generate_salt(int length = 8);
    static std::string hash_password(const std::string& password, const std::string& salt, const EVP_MD* md = EVP_sha256());
    static std::string generate_hashcode(const char* data, size_t n, const EVP_MD* md = EVP_sha256());
    static std::string generate_token(const User& user);
    static bool verify_token(const std::string& token, User& user);
private:
    /* 禁止构造对象 */
    CryptoUtil() = delete;
};
