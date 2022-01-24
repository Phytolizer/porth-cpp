#include "iota/iota.hpp"

#include <iostream>
#include <sstream>
#include <vector>

enum struct TokenKind
{
    SomethingElse,
    Identifier,
    EqualsSign,
    IotaKeyword,
    LeftBrace,
    RightBrace,
    Comma,
    Semicolon,
    ColonColon,
};

struct Token {
    TokenKind kind = TokenKind::Identifier;
    std::string text;
    Token() = default;
    Token(const TokenKind kind, std::string text) : kind(kind), text(std::move(text)) {
    }
};

struct Lexer {
    size_t start;
    size_t position;
    std::string text;

    explicit Lexer(std::string&& text) : start(0), position(0), text(std::move(text)) {
    }

    void startToken() {
        skipWhitespace();
        start = position;
    }

    [[nodiscard]] char current() const {
        return look(0);
    }

    [[nodiscard]] char peek() const {
        return look(1);
    }

    void operator++() {
        ++position;
    }

    [[nodiscard]] bool atEnd() const {
        return position >= text.length();
    }

    [[nodiscard]] bool atIdentifierStart() const {
        const char c = current();
        return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_';
    }

    [[nodiscard]] bool atIdentifierPart() const {
        const char c = current();
        return atIdentifierStart() || c >= '0' && c <= '9';
    }

    [[nodiscard]] std::string tokenText() const {
        return text.substr(start, position - start);
    }

  private:
    [[nodiscard]] bool atWhitespace() const {
        const char c = current();
        return c == ' ' || c == '\r' || c == '\n' || c == '\t';
    }

    void skipWhitespace() {
        while (atWhitespace()) {
            ++position;
        }
    }

    [[nodiscard]] char look(const size_t offset) const {
        const size_t index = position + offset;
        if (index >= text.length()) {
            return '\0';
        }
        return text[index];
    }
};

std::vector<Token> lex(const std::istream& input) {
    std::vector<Token> tokens;

    std::ostringstream sourceTextStream;
    sourceTextStream << input.rdbuf();
    Lexer lexer(sourceTextStream.str());
    while (!lexer.atEnd()) {
        lexer.startToken();
        auto kind = TokenKind::Identifier;
        std::string tokenText;
        if (lexer.current() == '=') {
            kind = TokenKind::EqualsSign;
            ++lexer;
        } else if (lexer.current() == '{') {
            kind = TokenKind::LeftBrace;
            ++lexer;
        } else if (lexer.current() == '}') {
            kind = TokenKind::RightBrace;
            ++lexer;
        } else if (lexer.current() == ',') {
            kind = TokenKind::Comma;
            ++lexer;
        } else if (lexer.current() == ':' && lexer.peek() == ':') {
            kind = TokenKind::ColonColon;
            ++lexer;
            ++lexer;
        } else if (lexer.atIdentifierStart()) {
            while (lexer.atIdentifierPart()) {
                ++lexer;
            }
            tokenText = lexer.tokenText();
            if (tokenText == "iota") {
                kind = TokenKind::IotaKeyword;
            } else {
                kind = TokenKind::Identifier;
            }
        } else {
            kind = TokenKind::SomethingElse;
            ++lexer;
        }

        if (tokenText.empty()) {
            tokenText = lexer.tokenText();
        }

        tokens.emplace_back(kind, std::move(tokenText));
    }

    return tokens;
}

struct Iota {
    bool parseSuccess = false;
    std::vector<Token> namespaces;
    Token name;
    std::vector<Token> variants;
};

Iota parse(const std::vector<Token>& tokens) {
    Iota i;
    auto it = tokens.begin();
    if (it == tokens.end() || it->kind != TokenKind::Identifier) {
        std::cerr << "iota parse error: must begin with an identifier\n";
        return i;
    }
    const Token* ident = &*it;
    ++it;
    if (it != tokens.end() && it->kind == TokenKind::ColonColon) {
        while (it != tokens.end() && it->kind == TokenKind::ColonColon) {
            i.namespaces.emplace_back(*ident);
            ++it;
            if (it == tokens.end() || it->kind != TokenKind::Identifier) {
                std::cerr << "iota parse error: '::' must be followed by an identifier\n";
                return i;
            }
            ident = &*it;
            ++it;
        }
        i.name = *ident;
    } else {
        i.name = *it;
        ++it;
    }
    if (it == tokens.end() || it->kind != TokenKind::EqualsSign) {
        std::cerr << "iota parse error: iota name must be followed by '='\n";
        return i;
    }
    ++it;
    if (it == tokens.end() || it->kind != TokenKind::IotaKeyword) {
        std::cerr << "iota parse error: '=' must be followed by 'iota'\n";
        return i;
    }
    ++it;
    if (it == tokens.end() || it->kind != TokenKind::LeftBrace) {
        std::cerr << "iota parse error: 'iota' must be followed by '{'\n";
        return i;
    }
    ++it;
    while (it != tokens.end() && it->kind != TokenKind::RightBrace) {
        if (it->kind != TokenKind::Identifier) {
            std::cerr << "iota parse error: variant name expected\n";
            return i;
        }
        i.variants.push_back(*it);
        ++it;
        if (it == tokens.end() || it->kind != TokenKind::Comma) {
            std::cerr << "iota parse error: comma expected after variant name\n";
            return i;
        }
        ++it;
    }
    if (it == tokens.end()) {
        std::cerr << "iota parse error: expected '}' at end of input\n";
        return i;
    }
    i.parseSuccess = true;
    return i;
}

std::string pluralize(const std::string& text) {
    std::string result;
    std::copy(text.begin(), text.end() - 2, std::back_inserter(result));
    if (result.ends_with('y')) {
        std::copy(text.end() - 2, text.end() - 1, std::back_inserter(result));
        result += "ies";
    } else if (result.ends_with('s') || result.ends_with("sh")) {
        std::copy(text.end() - 2, text.end(), std::back_inserter(result));
        result += "es";
    } else {
        std::copy(text.end() - 2, text.end(), std::back_inserter(result));
        result += "s";
    }
    return result;
}

void printNamespaces(std::ostream& output, const std::vector<Token>& namespaces) {
    for (size_t nsIdx = 0; nsIdx < namespaces.size(); ++nsIdx) {
        output << namespaces[nsIdx].text;
        if (nsIdx < namespaces.size() - 1) {
            output << "::";
        }
    }
}

void emitCode(std::ostream& output, const Iota& i) {
    output << "#pragma once\n";
    output << "#include <cstdint>\n";
    if (!i.namespaces.empty()) {
        output << "namespace ";
        printNamespaces(output, i.namespaces);
        output << " {\n";
    }
    output << "struct " << i.name.text << " {\n";
    output << "    std::uint64_t discriminant;\n";
    output << "    const char* name;\n";
    output << "    constexpr " << i.name.text
           << "(const std::uint64_t discriminant, const char* name) : discriminant(discriminant), name(name) {}\n";
    output << "    constexpr bool operator==(const " << i.name.text << "& other) const {\n";
    output << "        return discriminant == other.discriminant;\n";
    output << "    }\n";
    output << "};\n";
    const std::string namespaceName = pluralize(i.name.text);
    output << "namespace " << namespaceName << " {\n";
    size_t iotaValue = 0;
    for (const auto& [kind, text] : i.variants) {
        output << "constexpr " << i.name.text << " " << text << "{" << iotaValue << ", \"" << text << "\"};\n";
        ++iotaValue;
    }
    output << "constexpr " << i.name.text << " Count{" << iotaValue << ", \"Count\"};\n";
    output << "} // namespace " << namespaceName << "\n";
    if (!i.namespaces.empty()) {
        output << "} // namespace ";
        printNamespaces(output, i.namespaces);
        output << "\n";
    }
}

void iota::generate(const std::istream& input, std::ostream& output) {
    const std::vector<Token> tokens = lex(input);
    const Iota i = parse(tokens);
    emitCode(output, i);
}
