#pragma once

#include <string>
#include <vector>

namespace porth {

struct Token {
    std::string filePath;
    size_t lineNumber;
    size_t columnNumber;
    std::string token;
    Token(std::string filePath, const size_t lineNumber, const size_t columnNumber, std::string token)
        : filePath(std::move(filePath)), lineNumber(lineNumber), columnNumber(columnNumber), token(std::move(token)) {
    }
};

std::vector<Token> lexFile(const std::string& filePath);

} // namespace porth
