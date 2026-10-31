#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "entroui/parser/diagnostic.hpp"
#include "entroui/parser/token.hpp"

namespace EntroUI {

struct LexerOptions {
    std::size_t maxSourceSize = 4U * 1024U * 1024U;
};

class Lexer {
public:
    Lexer(std::string_view source, std::string filename = "<memory>",
          LexerOptions options = {});

    [[nodiscard]] std::vector<Token> tokenize(std::vector<Diagnostic>& diagnostics);

    [[nodiscard]] const std::string& filename() const noexcept { return filename_; }

private:
    [[nodiscard]] bool atEnd() const noexcept { return position_ >= source_.size(); }
    [[nodiscard]] char peek(std::size_t offset = 0) const noexcept;
    char advance();
    void advanceLine();
    [[nodiscard]] bool startsWith(std::string_view text) const noexcept;

    void skipWhitespace();
    Token lexTag();
    Token lexString();
    Token lexComment();
    Token lexText();
    Token lexIdentifier();

    void addError(std::vector<Diagnostic>& diagnostics, std::string message,
                  std::size_t line, std::size_t column) const;

    std::string_view source_;
    std::string filename_;
    LexerOptions options_;
    std::size_t position_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    std::vector<Diagnostic> pendingDiagnostics_;
};

} // namespace EntroUI
