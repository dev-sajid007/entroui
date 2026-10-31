#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "entroui/parser/diagnostic.hpp"
#include "entroui/style/selector.hpp"
#include "entroui/style/style.hpp"

namespace EntroUI {

class Element;

struct StyleDeclaration {
    std::string property;
    std::string value;
    bool important = false;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct StyleRule {
    std::vector<Selector> selectors;
    std::vector<StyleDeclaration> declarations;
    std::size_t order = 0;
};

struct CascadedValue {
    std::string value;
    int specificity = 0;
    std::size_t order = 0;
};

class StyleSheet {
public:
    StyleSheet() = default;

    [[nodiscard]] static ParseResult<StyleSheet> parse(std::string_view css,
                                                       std::string filename = "<style>");

    void addRule(StyleRule rule);
    [[nodiscard]] const std::vector<StyleRule>& rules() const noexcept { return rules_; }
    [[nodiscard]] std::size_t ruleCount() const noexcept { return rules_.size(); }

    void collectMatching(const Element& element,
                         std::unordered_map<std::string, CascadedValue>& values,
                         std::size_t orderOffset = 0) const;

    [[nodiscard]] static StyleSheet defaultTheme();

private:
    std::vector<StyleRule> rules_;
    std::size_t nextOrder_ = 0;
};

} // namespace EntroUI
