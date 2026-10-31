#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "entroui/core/document.hpp"
#include "entroui/parser/ast.hpp"
#include "entroui/parser/diagnostic.hpp"

namespace EntroUI {

struct BuildResult {
    std::unique_ptr<Document> document;
    std::optional<std::string> title;
    std::optional<int> width;
    std::optional<int> height;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool ok() const noexcept { return document != nullptr; }
};

[[nodiscard]] BuildResult buildDocument(const AstDocument& ast);

} // namespace EntroUI
