#include "Tokenizer.h"
#include <sstream>
#include <algorithm>

// cppjieba
#include "lib/cppjieba/cppjieba/include/cppjieba/Jieba.hpp"

static const char *const DICT_PATH = "lib/cppjieba/cppjieba/dict/jieba.dict.utf8";
static const char *const HMM_PATH = "lib/cppjieba/cppjieba/dict/hmm_model.utf8";
static const char *const USER_DICT_PATH = "lib/cppjieba/cppjieba/dict/user.dict.utf8";
static const char *const IDF_PATH = "lib/cppjieba/cppjieba/dict/idf.utf8";
static const char *const STOP_WORD_PATH = "lib/cppjieba/cppjieba/dict/stop_words.utf8";

static cppjieba::Jieba &jieba()
{
    static cppjieba::Jieba instance(DICT_PATH, HMM_PATH, USER_DICT_PATH,
                                     IDF_PATH, STOP_WORD_PATH);
    return instance;
}

std::string Tokenizer::normalize(const std::string &word)
{
    std::string result;
    result.reserve(word.size());
    for (char c : word) {
        if (c >= 'A' && c <= 'Z')
            result += (c - 'A' + 'a');
        else
            result += c;
    }
    return result;
}

std::vector<Token> Tokenizer::tokenize(const std::string &text)
{
    std::vector<Token> tokens;
    int pos = 0;

    // Detect if the text is mostly ASCII (English) or contains CJK
    int non_ascii = 0;
    for (char c : text)
        if (static_cast<unsigned char>(c) > 127) non_ascii++;

    if (non_ascii > text.size() / 4) {
        // Contains CJK → use jieba
        std::vector<std::string> words;
        jieba().Cut(text, words, true); // use HMM

        for (const auto &w : words) {
            std::string word = normalize(w);
            // Skip stop words (very short words)
            if (word.size() <= 1) continue;
            // Skip pure punctuation/whitespace
            bool is_valid = false;
            for (char c : word) {
                if (isalnum(c) || static_cast<unsigned char>(c) > 127)
                    { is_valid = true; break; }
            }
            if (!is_valid) continue;
            tokens.push_back({word, pos++});
        }
    } else {
        // English text → split by whitespace/punctuation
        std::string current;
        for (size_t i = 0; i <= text.size(); i++) {
            char c = (i < text.size()) ? text[i] : ' ';
            if (isalnum(c) || c == '-' || c == '_') {
                current += c;
            } else {
                if (!current.empty()) {
                    std::string word = normalize(current);
                    if (word.size() > 1)
                        tokens.push_back({word, pos++});
                    current.clear();
                }
            }
        }
    }

    return tokens;
}
