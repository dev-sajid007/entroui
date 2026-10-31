#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include "entroui/parser/ast.hpp"
#include "entroui/parser/diagnostic.hpp"

namespace EntroUI {

struct ParserOptions {
    std::size_t maxDepth = 256;
    std::size_t maxSourceSize = 4U * 1024U * 1024U;
};

class Parser {
public:
    explicit Parser(ParserOptions options = {});

    [[nodiscard]] ParseResult<AstDocument> parse(std::string_view source,
                                                 std::string filename = "<memory>") const;
    [[nodiscard]] ParseResult<AstDocument> parseFile(const std::filesystem::path& path) const;

    [[nodiscard]] const ParserOptions& options() const noexcept { return options_; }

private:
    ParserOptions options_;
};

} // namespace EntroUI
