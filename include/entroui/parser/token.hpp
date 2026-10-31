#pragma once

#include <cstddef>
#include <string>

namespace EntroUI {

enum class TokenType {
    TagOpen,
    TagClose,
    Slash,
    Equals,
    Identifier,
    String,
    Text,
    Comment,
    EndOfFile,
};

struct Token {
    TokenType type = TokenType::EndOfFile;
    std::string text;
    std::size_t line = 1;
    std::size_t column = 1;
};

[[nodiscard]] const char* tokenTypeName(TokenType type) noexcept;

} // namespace EntroUI
