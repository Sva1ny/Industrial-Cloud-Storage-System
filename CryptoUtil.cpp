#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <nlohmann/json.hpp>

#include "CryptoUtil.h"
#include "User.h"
#include "Config.h"

using namespace std;

// static 全局变量: 其它编译单元引用不了这个变量
static const std::string SECRET_KEY = jwt_secret();

// Base64url 编码 (RFC 4648 §5, 把 +/ 替换为 -_)
static string base64url_encode(const unsigned char* data, size_t len)
{
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    string result;
    result.reserve((len + 2) / 3 * 4);
    for (size_t i = 0; i < len; i += 3) {
        unsigned int n = (unsigned)data[i] << 16;
        if (i + 1 < len) n |= data[i + 1] << 8;
        if (i + 2 < len) n |= data[i + 2];
        result += b64[(n >> 18) & 0x3f];
        result += b64[(n >> 12) & 0x3f];
        result += (i + 1 < len) ? b64[(n >> 6) & 0x3f] : '=';
        result += (i + 2 < len) ? b64[n & 0x3f] : '=';
    }
    // 移除 padding (JWT 标准不使用 = padding)
    while (!result.empty() && result.back() == '=') result.pop_back();
    return result;
}

static int b64dec_char(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-' || c == '+') return 62;
    if (c == '_' || c == '/') return 63;
    return -1;
}

// Base64url 解码
static string base64url_decode(const string& input)
{
    string normalized = input;
    // 恢复 padding
    while (normalized.size() % 4) normalized += '=';

    size_t len = normalized.size();
    string result;
    result.reserve(len / 4 * 3);
    for (size_t i = 0; i < len; i += 4) {
        int a = b64dec_char(normalized[i]);
        int b = b64dec_char(normalized[i + 1]);
        int c = b64dec_char(normalized[i + 2]);
        int d = b64dec_char(normalized[i + 3]);
        result += (char)((a << 2) | (b >> 4));
        if (normalized[i + 2] != '=') result += (char)(((b & 0xf) << 4) | (c >> 2));
        if (normalized[i + 3] != '=') result += (char)(((c & 0x3) << 6) | d);
    }
    return result;
}

string CryptoUtil::generate_salt(int length)
{
    const char* alpha = "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    string result;
    for (int i = 0; i < length; ++i) {
        result += alpha[rand() % 62];
    }
    return result;
}

// OpenSSL 3.0 及更新版本推荐使用 EVP(Envelope) 接口
string CryptoUtil::hash_password(const string& password, const string& salt, const EVP_MD* md)
{
    EVP_MD_CTX* context = EVP_MD_CTX_new(); // 创建 EVP 上下文

    EVP_DigestInit_ex(context, md, NULL);
    // 更新上下文
    EVP_DigestUpdate(context, salt.c_str(), salt.size());
    EVP_DigestUpdate(context, password.c_str(), password.size());
    // 计算哈希值
    unsigned char hash[EVP_MAX_MD_SIZE];    // 最大哈希长度
    unsigned int len = 0;                   // 用来接收实际哈希长度
    EVP_DigestFinal(context, hash, &len);

    // 转换成十六进制字符，存储到result中
    char result[EVP_MAX_MD_SIZE * 2 + 1] = { '\0' };
    for(unsigned i = 0; i < len; i++) {
        sprintf(result + 2 * i, "%02x", hash[i]);
    }

    EVP_MD_CTX_free(context);               // 释放上下文

    return result;
}

string CryptoUtil::generate_token(const User& user)
{
    // 1. 构建 header
    nlohmann::json header = nlohmann::json::object();
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    // 2. 构建 payload
    nlohmann::json payload = nlohmann::json::object();
    payload["sub"] = "LoginToken";
    payload["id"] = user.id;
    payload["username"] = user.username;
    payload["created_at"] = user.createdAt;
    payload["exp"] = time(NULL) + 3600;

    // 3. Base64url 编码 header 和 payload
    string b64_header = base64url_encode(
        (const unsigned char*)header.dump().c_str(), header.dump().size());
    string b64_payload = base64url_encode(
        (const unsigned char*)payload.dump().c_str(), payload.dump().size());

    // 4. HMAC-SHA256 签名
    string signing_input = b64_header + "." + b64_payload;
    unsigned char sig[EVP_MAX_MD_SIZE];
    unsigned int sig_len = 0;
    HMAC(EVP_sha256(),
         SECRET_KEY.c_str(), (int)SECRET_KEY.size(),
         (const unsigned char*)signing_input.c_str(), signing_input.size(),
         sig, &sig_len);

    // 5. Base64url 编码签名
    string b64_sig = base64url_encode(sig, sig_len);

    return b64_header + "." + b64_payload + "." + b64_sig;
}

bool CryptoUtil::verify_token(const string& token, User& user)
{
    // 1. 分割 token
    size_t dot1 = token.find('.');
    size_t dot2 = token.rfind('.');
    if (dot1 == string::npos || dot2 == string::npos || dot1 == dot2) {
        return false;
    }

    string b64_header = token.substr(0, dot1);
    string b64_payload = token.substr(dot1 + 1, dot2 - dot1 - 1);
    string b64_sig = token.substr(dot2 + 1);

    // 2. 验证签名
    string signing_input = b64_header + "." + b64_payload;
    unsigned char expected_sig[EVP_MAX_MD_SIZE];
    unsigned int expected_len = 0;
    HMAC(EVP_sha256(),
         SECRET_KEY.c_str(), (int)SECRET_KEY.size(),
         (const unsigned char*)signing_input.c_str(), signing_input.size(),
         expected_sig, &expected_len);

    string expected_b64_sig = base64url_encode(expected_sig, expected_len);
    if (expected_b64_sig != b64_sig) {
        return false;
    }

    // 3. 解析 payload
    string payload_json = base64url_decode(b64_payload);
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(payload_json);
    } catch (...) {
        return false;
    }

    // 4. 验证 subject
    if (payload["sub"] != "LoginToken") {
        return false;
    }

    // 5. 验证过期时间
    if (payload.contains("exp") && payload["exp"].is_number()) {
        long expire = payload["exp"].get<long>();
        if (expire < time(NULL)) {
            return false;
        }
    }

    // 6. 提取用户信息
    user.id = payload.value("id", 0);
    user.username = payload.value("username", string());
    user.createdAt = payload.value("created_at", string());

    return true;
}

std::string CryptoUtil::generate_hashcode(const char* data, size_t n, const EVP_MD* md)
{
    EVP_MD_CTX* context = EVP_MD_CTX_new();     // 创建 EVP 上下文

    EVP_DigestInit_ex(context, md, NULL);

    EVP_DigestUpdate(context, data, n);

    // 计算哈希值
    unsigned char hash[EVP_MAX_MD_SIZE];    // 最大哈希长度
    unsigned int len = 0;                   // 用来接收实际哈希长度
    EVP_DigestFinal(context, hash, &len);

    char result[EVP_MAX_MD_SIZE * 2 + 1] = { '\0' };
    // 转换成十六进制字符，存储到output中
    for (unsigned i = 0; i < len; i++) {
        sprintf(result + 2 * i, "%02x", hash[i]);
    }
    EVP_MD_CTX_free(context);               // 释放上下文

    return result;
}
