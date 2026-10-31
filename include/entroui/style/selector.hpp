#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "entroui/parser/diagnostic.hpp"

namespace EntroUI {

class Element;

struct Selector {
    std::optional<std::string> tag;
    std::optional<std::string> id;
    std::vector<std::string> classes;

    [[nodiscard]] int specificity() const noexcept;
    [[nodiscard]] bool matches(const Element& element) const;
    [[nodiscard]] std::string toString() const;
};

[[nodiscard]] ParseResult<Selector> parseSelector(std::string_view text,
                                                  std::string file = "<style>",
                                                  std::size_t line = 1,
                                                  std::size_t column = 1);

} // namespace EntroUI
