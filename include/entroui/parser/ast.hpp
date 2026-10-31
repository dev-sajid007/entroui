#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace EntroUI {

struct AstAttribute {
    std::string name;
    std::string value;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct AstNode {
    enum class Kind { Element, Text };

    Kind kind = Kind::Element;
    std::string name;
    std::string text;
    std::vector<AstAttribute> attributes;
    std::vector<std::unique_ptr<AstNode>> children;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct AstDocument {
    std::vector<std::unique_ptr<AstNode>> roots;
};

} // namespace EntroUI
