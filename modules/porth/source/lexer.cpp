#include "porth/lexer.hpp"

#include <fstream>
#include <ranges/ranges.hpp>
#include <sstream>

std::string_view::iterator trimLeft(std::string_view line, const std::string_view::iterator col) {
    return std::find_if(col, line.end(), [](const char c) { return !std::isspace(c); });
}

porth::Token lexWord(
    const std::string& filePath,
    const std::size_t lineNumber,
    const std::size_t columnNumber,
    const std::string& text) {
    for (const char c : text) {
        if (!std::isdigit(c)) {
            return {porth::TokenIds::Word, filePath, lineNumber, columnNumber, text};
        }
    }
    return {porth::TokenIds::Int, filePath, lineNumber, columnNumber, text};
}

std::vector<porth::Token> lexLine(const std::string& filePath, const size_t lineNumber, std::string_view line) {
    std::vector<porth::Token> result;
    auto col = trimLeft(line, line.begin());
    while (col != line.end()) {
        const auto colEnd = std::find_if(col, line.end(), [](const char c) { return std::isspace(c); });
        std::string tokenText{col, colEnd};
        if (tokenText == "//") {
            break;
        }
        result.emplace_back(lexWord(filePath, lineNumber + 1, col - line.begin() + 1, tokenText));
        col = trimLeft(line, colEnd);
    }
    return result;
}

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
        my_ranges::copy(lexLine(filePath, lineNumber, line), std::back_inserter(result));
        ++lineNumber;
    }
    return result;
}
