#include "porth/lexer.hpp"

#include "porth/op.hpp"

#include <fstream>
#include <sstream>
#include <stack>

std::string_view::iterator trimLeft(std::string_view line, const std::string_view::iterator col) {
    return std::find_if(col, line.end(), [](const char c) { return !std::isspace(c); });
}

std::vector<porth::Token> lexLine(const std::string& filePath, const size_t lineNumber, std::string_view line) {
    std::vector<porth::Token> result;
    auto col = trimLeft(line, line.begin());
    while (col != line.end()) {
        const auto colEnd = std::find_if(col, line.end(), [](const char c) { return std::isspace(c); });
        result.emplace_back(filePath, lineNumber + 1, col - line.begin() + 1, std::string{col, colEnd});
        col = trimLeft(line, colEnd);
    }
    return result;
}

struct LexedLine {
    size_t lineNumber;
    std::vector<porth::Token> line;
    constexpr LexedLine(const size_t lineNumber, std::vector<porth::Token> line)
        : lineNumber(lineNumber), line(std::move(line)) {
    }
};

std::vector<porth::Token> porth::lexFile(const std::string& filePath) {
    std::vector<Token> result;
    const std::ifstream file{filePath};
    if (!file) {
        std::ostringstream errorMessage;
        errorMessage << "failed to open " << filePath << " for reading";
        throw std::runtime_error{errorMessage.str()};
    }
    std::stringstream source;
    source << file.rdbuf();
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(source, line)) {
        std::ranges::copy(lexLine(filePath, lineNumber, line), std::back_inserter(result));
        ++lineNumber;
    }
    return result;
}
