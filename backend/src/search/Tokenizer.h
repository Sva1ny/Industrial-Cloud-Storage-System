#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

struct Token {
    std::string word;
    int position;
};

class Tokenizer {
public:
    // Split text into tokens (Chinese via jieba, English by whitespace)
    static std::vector<Token> tokenize(const std::string &text);

    // Normalize a word (lowercase, trim)
    static std::string normalize(const std::string &word);
};
