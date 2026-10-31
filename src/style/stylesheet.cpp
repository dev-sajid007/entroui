#include "entroui/style/stylesheet.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>

#include "entroui/core/element.hpp"
#include "entroui/style/computed_style.hpp"

namespace EntroUI {

namespace {

constexpr std::size_t kMaxStylesheetSize = 4U * 1024U * 1024U;

std::string trimCopy(std::string_view value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return std::string(value.substr(begin, end - begin));
}

std::string lowerCopy(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

std::vector<std::string> splitSelectorList(std::string_view value) {
    std::vector<std::string> selectors;
    std::size_t start = 0;
    while (start <= value.size()) {
        const std::size_t comma = value.find(',', start);
        const std::size_t end = comma == std::string_view::npos ? value.size() : comma;
        selectors.push_back(trimCopy(value.substr(start, end - start)));
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return selectors;
}

class CssScanner {
public:
    CssScanner(std::string_view source, std::string filename)
        : source_(source), filename_(std::move(filename)) {}

    [[nodiscard]] bool eof() const noexcept { return position_ >= source_.size(); }
    [[nodiscard]] std::size_t line() const noexcept { return line_; }
    [[nodiscard]] std::size_t column() const noexcept { return column_; }
    [[nodiscard]] const std::string& filename() const noexcept { return filename_; }

    char peek(std::size_t offset = 0) const noexcept {
        const std::size_t position = position_ + offset;
        return position < source_.size() ? source_[position] : '\0';
    }

    char advance() {
        const char character = source_[position_++];
        if (character == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }
        return character;
    }

    void skipSpaceAndComments(std::vector<Diagnostic>& diagnostics) {
        while (!eof()) {
            if (std::isspace(static_cast<unsigned char>(peek())) != 0) {
                advance();
                continue;
            }
            if (peek() == '/' && peek(1) == '*') {
                const std::size_t startLine = line_;
                const std::size_t startColumn = column_;
                advance();
                advance();
                bool closed = false;
                while (!eof()) {
                    if (peek() == '*' && peek(1) == '/') {
                        advance();
                        advance();
                        closed = true;
                        break;
                    }
                    advance();
                }
                if (!closed) {
                    diagnostics.push_back(
                        makeError("Unterminated CSS comment", filename_, startLine, startColumn));
                }
                continue;
            }
            break;
        }
    }

    [[nodiscard]] std::string readUntil(std::string_view delimiters) {
        std::string result;
        while (!eof()) {
            const char character = peek();
            if (delimiters.find(character) != std::string_view::npos) {
                break;
            }
            result.push_back(advance());
        }
        return result;
    }

private:
    std::string_view source_;
    std::string filename_;
    std::size_t position_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
};

std::string readDeclarationValue(CssScanner& scanner) {
    std::string value;
    int parentheses = 0;
    while (!scanner.eof()) {
        const char character = scanner.peek();
        if (character == '(') {
            ++parentheses;
        } else if (character == ')') {
            parentheses = std::max(0, parentheses - 1);
        }
        if (parentheses == 0 && (character == ';' || character == '}')) {
            break;
        }
        value.push_back(scanner.advance());
    }
    return value;
}

} // namespace

ParseResult<StyleSheet> StyleSheet::parse(std::string_view css, std::string filename) {
    ParseResult<StyleSheet> result;
    StyleSheet sheet;

    if (css.size() > kMaxStylesheetSize) {
        result.diagnostics.push_back(makeError("Stylesheet exceeds the maximum size", filename, 1,
                                               1));
        return result;
    }

    CssScanner scanner(css, std::move(filename));

    while (!scanner.eof()) {
        scanner.skipSpaceAndComments(result.diagnostics);
        if (scanner.eof()) {
            break;
        }

        if (scanner.peek() == '}') {
            result.diagnostics.push_back(
                makeError("Unexpected '}'", scanner.filename(), scanner.line(), scanner.column()));
            scanner.advance();
            continue;
        }

        const std::size_t selectorLine = scanner.line();
        const std::size_t selectorColumn = scanner.column();
        const std::string selectorText = scanner.readUntil("{};");
        if (scanner.eof() || scanner.peek() != '{') {
            result.diagnostics.push_back(makeError("Expected '{' after selector", scanner.filename(),
                                                   scanner.line(), scanner.column()));
            if (!scanner.eof()) {
                scanner.advance();
            }
            continue;
        }
        scanner.advance();

        StyleRule rule;
        for (const auto& selectorPart : splitSelectorList(selectorText)) {
            auto parsed = parseSelector(selectorPart, scanner.filename(), selectorLine,
                                        selectorColumn);
            if (parsed.ok()) {
                rule.selectors.push_back(std::move(*parsed.value));
            }
            for (auto& diagnostic : parsed.diagnostics) {
                result.diagnostics.push_back(std::move(diagnostic));
            }
        }

        if (rule.selectors.empty()) {
            while (!scanner.eof() && scanner.peek() != '}') {
                scanner.advance();
            }
            if (!scanner.eof()) {
                scanner.advance();
            }
            continue;
        }

        bool closed = false;
        while (!scanner.eof()) {
            scanner.skipSpaceAndComments(result.diagnostics);
            if (scanner.eof()) {
                break;
            }
            if (scanner.peek() == '}') {
                scanner.advance();
                closed = true;
                break;
            }

            const std::size_t declarationLine = scanner.line();
            const std::size_t declarationColumn = scanner.column();
            const std::string propertyText = scanner.readUntil(":;{}");
            if (scanner.eof() || scanner.peek() != ':') {
                result.diagnostics.push_back(
                    makeError("Expected ':' after property name", scanner.filename(),
                              scanner.line(), scanner.column()));
                while (!scanner.eof() && scanner.peek() != ';' && scanner.peek() != '}') {
                    scanner.advance();
                }
                if (!scanner.eof() && scanner.peek() == ';') {
                    scanner.advance();
                }
                continue;
            }
            scanner.advance();

            std::string value = trimCopy(readDeclarationValue(scanner));
            bool important = false;
            const std::string lower = lowerCopy(value);
            const std::string suffix = "!important";
            if (lower.size() >= suffix.size() &&
                lower.compare(lower.size() - suffix.size(), suffix.size(), suffix) == 0) {
                important = true;
                value = trimCopy(value.substr(0, value.size() - suffix.size()));
            }
            if (!scanner.eof() && scanner.peek() == ';') {
                scanner.advance();
            }

            const std::string property = lowerCopy(trimCopy(propertyText));
            if (property.empty()) {
                continue;
            }
            if (!isValidStyleProperty(property)) {
                result.diagnostics.push_back(makeWarning("Unknown CSS property '" + property + "'",
                                                         scanner.filename(), declarationLine,
                                                         declarationColumn));
                continue;
            }
            StyleProperties validation;
            if (value.empty() ||
                !applyStyleDeclaration(validation, property, value)) {
                result.diagnostics.push_back(makeWarning(
                    "Invalid value for '" + property + "': " + value, scanner.filename(),
                    declarationLine, declarationColumn));
                continue;
            }
            rule.declarations.push_back(
                StyleDeclaration{property, value, important, declarationLine, declarationColumn});
        }

        if (!closed) {
            result.diagnostics.push_back(makeError("Missing closing '}'", scanner.filename(),
                                                   selectorLine, selectorColumn));
        }
        sheet.addRule(std::move(rule));
    }

    result.value = std::move(sheet);
    return result;
}

void StyleSheet::addRule(StyleRule rule) {
    rule.order = nextOrder_++;
    rules_.push_back(std::move(rule));
}

void StyleSheet::collectMatching(const Element& element,
                                 std::unordered_map<std::string, CascadedValue>& values,
                                 std::size_t orderOffset) const {
    for (const auto& rule : rules_) {
        int bestSpecificity = -1;
        for (const auto& selector : rule.selectors) {
            if (selector.matches(element)) {
                bestSpecificity = std::max(bestSpecificity, selector.specificity());
            }
        }
        if (bestSpecificity < 0) {
            continue;
        }
        std::size_t declarationIndex = 0;
        for (const auto& declaration : rule.declarations) {
            const CascadedValue candidate{
                declaration.value,
                bestSpecificity,
                orderOffset + rule.order * 10000U + declarationIndex,
            };
            ++declarationIndex;
            auto it = values.find(declaration.property);
            if (it == values.end()) {
                values.emplace(declaration.property, candidate);
                continue;
            }
            if (candidate.specificity > it->second.specificity ||
                (candidate.specificity == it->second.specificity &&
                 candidate.order >= it->second.order)) {
                it->second = candidate;
            }
        }
    }
}

StyleSheet StyleSheet::defaultTheme() {
    const auto shared = defaultThemeStylesheet();
    return *shared;
}

} // namespace EntroUI
