#include "iota/iota.hpp"

#include <iostream>
#include <regex>
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

struct ParserError final : std::runtime_error {
    explicit ParserError(const std::string& what) : std::runtime_error(what) {
    }
};

struct Parser {
    Iota iota;
    const std::vector<Token>& tokens;
    std::vector<Token>::const_iterator it;

    explicit Parser(const std::vector<Token>& tokens) : tokens{tokens}, it{tokens.begin()} {
    }

    const Token& consume(const TokenKind kind, const std::string& message) {
        if (!atToken(kind)) {
            throw ParserError{message};
        }
        return next();
    }

    [[nodiscard]] bool atToken(const TokenKind kind) const {
        return it != tokens.end() && it->kind == kind;
    }

    void pushNamespace(Token&& ns) {
        iota.namespaces.emplace_back(std::move(ns));
    }

    void pushVariant(const Token& variant) {
        iota.variants.emplace_back(variant);
    }

    void operator++() {
        ++it;
    }

    const Token& next() {
        return *it++;
    }

    [[nodiscard]] bool atEnd() const {
        return it == tokens.end();
    }

    [[nodiscard]] Iota&& complete() {
        if (!atEnd()) {
            throw ParserError{"expected end of input"};
        }
        return std::move(iota);
    }
};

Iota parse(const std::vector<Token>& tokens) {
    Parser parser{tokens};
    Token ident = parser.consume(TokenKind::Identifier, "must begin with an identifier");
    if (parser.atToken(TokenKind::ColonColon)) {
        while (parser.atToken(TokenKind::ColonColon)) {
            parser.pushNamespace(std::move(ident));
            ++parser;
            ident = parser.consume(TokenKind::Identifier, "'::' must be followed by an identifier");
            ++parser;
        }
        parser.iota.name = std::move(ident);
    } else {
        parser.iota.name = parser.next();
    }
    parser.consume(TokenKind::EqualsSign, "iota name must be followed by '='");
    parser.consume(TokenKind::IotaKeyword, "'=' must be followed by 'iota'");
    parser.consume(TokenKind::LeftBrace, "'iota' must be followed by '{'");
    while (!parser.atEnd() && !parser.atToken(TokenKind::RightBrace)) {
        parser.pushVariant(parser.consume(TokenKind::Identifier, "variant name expected"));
        parser.consume(TokenKind::Comma, "comma expected after variant name");
    }
    parser.consume(TokenKind::RightBrace, "expected '}' after variant names");
    parser.consume(TokenKind::Semicolon, "expected ';' after '}'");
    return parser.complete();
}

std::string pluralize(const std::string& text) {
    if (std::regex_search(text, std::regex{"y$"})) {
        return std::regex_replace(text, std::regex{"y$"}, "ies");
    }
    if (std::regex_search(text, std::regex{"sh?$"})) {
        return text + "es";
    }
    return text + "s";
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

int iota::generate(const std::istream& input, std::ostream& output) {
    const std::vector<Token> tokens = lex(input);
    try {
        emitCode(output, parse(tokens));
    } catch (const ParserError& e) {
        std::cerr << "iota parse error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
