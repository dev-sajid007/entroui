#include "entroui/style/selector.hpp"

#include <cctype>

#include "entroui/core/element.hpp"

namespace EntroUI {

namespace {

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

bool isIdentifierChar(char character) {
    const unsigned char value = static_cast<unsigned char>(character);
    return std::isalnum(value) != 0 || character == '-' || character == '_';
}

ParseResult<Selector> failure(std::string message, const std::string& file, std::size_t line,
                              std::size_t column) {
    ParseResult<Selector> result;
    result.diagnostics.push_back(makeError(std::move(message), file, line, column));
    return result;
}

} // namespace

int Selector::specificity() const noexcept {
    int value = 0;
    if (id) {
        value += 100;
    }
    value += static_cast<int>(classes.size()) * 10;
    if (tag) {
        value += 1;
    }
    return value;
}

bool Selector::matches(const Element& element) const {
    if (tag && element.tag() != *tag) {
        return false;
    }
    if (id && element.id() != *id) {
        return false;
    }
    for (const auto& className : classes) {
        if (!element.hasClass(className)) {
            return false;
        }
    }
    return true;
}

std::string Selector::toString() const {
    std::string result;
    if (tag) {
        result += *tag;
    }
    if (id) {
        result += '#' + *id;
    }
    for (const auto& className : classes) {
        result += '.' + className;
    }
    return result.empty() ? "*" : result;
}

ParseResult<Selector> parseSelector(std::string_view text, std::string file, std::size_t line,
                                    std::size_t column) {
    const std::string trimmed = trimCopy(text);
    if (trimmed.empty()) {
        return failure("Empty selector", file, line, column);
    }
    for (char character : trimmed) {
        if (std::isspace(static_cast<unsigned char>(character)) != 0 || character == '>' ||
            character == '+' || character == '~' || character == ':' || character == '[' ||
            character == ']' || character == '(' || character == ')') {
            return failure("Unsupported selector syntax: " + trimmed, file, line, column);
        }
    }

    Selector selector;
    std::size_t position = 0;
    if (trimmed[position] == '*') {
        ++position;
    } else if (trimmed[position] != '.' && trimmed[position] != '#') {
        const std::size_t start = position;
        while (position < trimmed.size() && isIdentifierChar(trimmed[position])) {
            ++position;
        }
        if (position == start) {
            return failure("Invalid tag selector: " + trimmed, file, line, column);
        }
        selector.tag = trimmed.substr(start, position - start);
    }

    while (position < trimmed.size()) {
        const char marker = trimmed[position];
        if (marker != '.' && marker != '#') {
            return failure("Unexpected character in selector: " + trimmed, file, line, column);
        }
        ++position;
        const std::size_t start = position;
        while (position < trimmed.size() && isIdentifierChar(trimmed[position])) {
            ++position;
        }
        if (position == start) {
            return failure("Missing selector name: " + trimmed, file, line, column);
        }
        const std::string name = trimmed.substr(start, position - start);
        if (marker == '.') {
            selector.classes.push_back(name);
        } else {
            if (selector.id) {
                return failure("Selector cannot contain more than one id", file, line, column);
            }
            selector.id = name;
        }
    }

    if (!selector.tag && !selector.id && selector.classes.empty()) {
        return failure("Invalid selector: " + trimmed, file, line, column);
    }

    ParseResult<Selector> result;
    result.value = std::move(selector);
    return result;
}

} // namespace EntroUI
