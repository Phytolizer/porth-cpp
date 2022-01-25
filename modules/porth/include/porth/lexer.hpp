#pragma once

#include <iota_generated/token_id.hpp>
#include <string>
#include <vector>

namespace porth {

struct Token {
    TokenId id;
    std::string filePath;
    size_t lineNumber;
    size_t columnNumber;
    std::string token;
    Token(const TokenId id, std::string filePath, const size_t lineNumber, const size_t columnNumber, std::string token)
        : id(id), filePath(std::move(filePath)), lineNumber(lineNumber), columnNumber(columnNumber),
          token(std::move(token)) {
    }
};

std::vector<Token> lexFile(const std::string& filePath);

} // namespace porth
