#include "entroui/parser/lexer.hpp"

#include <cctype>

namespace EntroUI {

const char* tokenTypeName(TokenType type) noexcept {
    switch (type) {
    case TokenType::TagOpen:
        return "tag-open";
    case TokenType::TagClose:
        return "tag-close";
    case TokenType::Slash:
        return "slash";
    case TokenType::Equals:
        return "equals";
    case TokenType::Identifier:
        return "identifier";
    case TokenType::String:
        return "string";
    case TokenType::Text:
        return "text";
    case TokenType::Comment:
        return "comment";
    case TokenType::EndOfFile:
        return "end-of-file";
    }
    return "unknown";
}

Lexer::Lexer(std::string_view source, std::string filename, LexerOptions options)
    : source_(source), filename_(std::move(filename)), options_(options) {}

bool Lexer::startsWith(std::string_view text) const noexcept {
    return source_.substr(position_, text.size()) == text;
}

char Lexer::peek(std::size_t offset) const noexcept {
    const std::size_t index = position_ + offset;
    return index < source_.size() ? source_[index] : '\0';
}

char Lexer::advance() {
    const char character = source_[position_++];
    if (character == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return character;
}

void Lexer::advanceLine() {
    advance();
}

void Lexer::addError(std::vector<Diagnostic>& diagnostics, std::string message,
                     std::size_t line, std::size_t column) const {
    diagnostics.push_back(makeError(std::move(message), filename_, line, column));
}

std::vector<Token> Lexer::tokenize(std::vector<Diagnostic>& diagnostics) {
    std::vector<Token> tokens;
    pendingDiagnostics_.clear();
    if (source_.size() > options_.maxSourceSize) {
        addError(diagnostics, "Markup source exceeds the maximum size", 1, 1);
        tokens.push_back({TokenType::EndOfFile, {}, line_, column_});
        return tokens;
    }

    while (!atEnd()) {
        if (startsWith("<!--")) {
            tokens.push_back(lexComment());
            continue;
        }
        if (startsWith("<!DOCTYPE") || startsWith("<!doctype")) {
            const std::size_t tokenLine = line_;
            const std::size_t tokenColumn = column_;
            while (!atEnd() && peek() != '>') {
                advance();
            }
            if (!atEnd()) {
                advance();
            } else {
                addError(diagnostics, "Unterminated declaration", tokenLine, tokenColumn);
            }
            continue;
        }
        if (peek() == '<') {
            Token open{TokenType::TagOpen, "<", line_, column_};
            tokens.push_back(std::move(open));
            advance();
            bool tagClosed = false;
            while (!atEnd()) {
                const char character = peek();
                if (std::isspace(static_cast<unsigned char>(character)) != 0) {
                    advance();
                    continue;
                }
                if (character == '>') {
                    tokens.push_back({TokenType::TagClose, ">", line_, column_});
                    advance();
                    tagClosed = true;
                    break;
                }
                if (character == '/') {
                    tokens.push_back({TokenType::Slash, "/", line_, column_});
                    advance();
                    continue;
                }
                if (character == '=') {
                    tokens.push_back({TokenType::Equals, "=", line_, column_});
                    advance();
                    continue;
                }
                if (character == '"' || character == '\'') {
                    tokens.push_back(lexString());
                    continue;
                }
                if (std::isalpha(static_cast<unsigned char>(character)) != 0 ||
                    character == '_' || character == '-') {
                    tokens.push_back(lexIdentifier());
                    continue;
                }
                addError(diagnostics,
                         std::string("Unexpected character '") + character + "' inside a tag",
                         line_, column_);
                advance();
            }
            if (!tagClosed) {
                addError(diagnostics, "Unterminated tag", line_, column_);
            }
            continue;
        }
        tokens.push_back(lexText());
    }

    for (auto& diagnostic : pendingDiagnostics_) {
        diagnostics.push_back(std::move(diagnostic));
    }
    tokens.push_back({TokenType::EndOfFile, {}, line_, column_});
    return tokens;
}

Token Lexer::lexText() {
    const std::size_t tokenLine = line_;
    const std::size_t tokenColumn = column_;
    std::string text;
    while (!atEnd() && peek() != '<') {
        text.push_back(advance());
    }
    return {TokenType::Text, std::move(text), tokenLine, tokenColumn};
}

Token Lexer::lexIdentifier() {
    const std::size_t tokenLine = line_;
    const std::size_t tokenColumn = column_;
    std::string text;
    while (!atEnd()) {
        const char character = peek();
        if (std::isalnum(static_cast<unsigned char>(character)) == 0 && character != '-' &&
            character != '_' && character != ':') {
            break;
        }
        text.push_back(advance());
    }
    return {TokenType::Identifier, std::move(text), tokenLine, tokenColumn};
}

Token Lexer::lexString() {
    const std::size_t tokenLine = line_;
    const std::size_t tokenColumn = column_;
    const char quote = advance();
    std::string text;
    bool terminated = false;
    while (!atEnd()) {
        const char character = peek();
        if (character == quote) {
            advance();
            terminated = true;
            break;
        }
        if (character == '\\') {
            advance();
            if (atEnd()) {
                break;
            }
            const char escaped = advance();
            switch (escaped) {
            case 'n':
                text.push_back('\n');
                break;
            case 't':
                text.push_back('\t');
                break;
            case 'r':
                text.push_back('\r');
                break;
            default:
                text.push_back(escaped);
                break;
            }
            continue;
        }
        text.push_back(advance());
    }
    if (!terminated) {
        pendingDiagnostics_.push_back(
            makeError("Unterminated quoted attribute value", filename_, tokenLine, tokenColumn));
    }
    return {TokenType::String, std::move(text), tokenLine, tokenColumn};
}

Token Lexer::lexComment() {
    const std::size_t tokenLine = line_;
    const std::size_t tokenColumn = column_;
    for (int index = 0; index < 4; ++index) {
        advance();
    }
    std::string text;
    bool closed = false;
    while (!atEnd()) {
        if (startsWith("-->")) {
            advance();
            advance();
            advance();
            closed = true;
            break;
        }
        text.push_back(advance());
    }
    if (!closed) {
        pendingDiagnostics_.push_back(
            makeError("Unterminated comment", filename_, tokenLine, tokenColumn));
    }
    return {TokenType::Comment, std::move(text), tokenLine, tokenColumn};
}

void Lexer::skipWhitespace() {
    while (!atEnd() && std::isspace(static_cast<unsigned char>(peek())) != 0) {
        advance();
    }
}

} // namespace EntroUI
