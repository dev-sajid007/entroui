#include "entroui/parser/builder.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

#include "entroui/core/element.hpp"
#include "entroui/core/text_node.hpp"

namespace EntroUI {

namespace {

constexpr std::size_t kMaxBuildDepth = 256;

std::string lowerCopy(std::string_view value) {
    std::string result(value);
    for (char& character : result) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return result;
}

std::string decodeEntities(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    std::size_t position = 0;
    while (position < text.size()) {
        if (text[position] != '&') {
            result.push_back(text[position]);
            ++position;
            continue;
        }
        const std::size_t semicolon = text.find(';', position + 1);
        if (semicolon == std::string_view::npos || semicolon - position > 12) {
            result.push_back('&');
            ++position;
            continue;
        }
        const std::string entity(text.substr(position + 1, semicolon - position - 1));
        if (entity == "amp") {
            result.push_back('&');
        } else if (entity == "lt") {
            result.push_back('<');
        } else if (entity == "gt") {
            result.push_back('>');
        } else if (entity == "quot") {
            result.push_back('"');
        } else if (entity == "apos") {
            result.push_back('\'');
        } else if (!entity.empty() && entity.front() == '#') {
            unsigned long code = 0;
            bool valid = false;
            if (entity.size() > 2 && (entity[1] == 'x' || entity[1] == 'X')) {
                code = std::strtoul(entity.c_str() + 2, nullptr, 16);
                valid = code > 0;
            } else {
                code = std::strtoul(entity.c_str() + 1, nullptr, 10);
                valid = code > 0;
            }
            if (valid && code <= 0x10FFFF) {
                if (code < 0x80) {
                    result.push_back(static_cast<char>(code));
                } else if (code < 0x800) {
                    result.push_back(static_cast<char>(0xC0 | (code >> 6)));
                    result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                } else if (code < 0x10000) {
                    result.push_back(static_cast<char>(0xE0 | (code >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                } else {
                    result.push_back(static_cast<char>(0xF0 | (code >> 18)));
                    result.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                }
            }
        } else {
            result.append(text.substr(position, semicolon - position + 1));
        }
        position = semicolon + 1;
    }
    return result;
}

std::string normalizeText(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    bool lastWasSpace = false;
    for (char character : text) {
        const bool isSpace =
            character == ' ' || character == '\t' || character == '\n' || character == '\r';
        if (isSpace) {
            if (!result.empty() && !lastWasSpace) {
                result.push_back(' ');
            }
            lastWasSpace = true;
        } else {
            result.push_back(character);
            lastWasSpace = false;
        }
    }
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

std::optional<int> parsePositiveInt(std::string_view value) {
    int result = 0;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto [pointer, error] = std::from_chars(begin, end, result);
    if (error != std::errc() || pointer != end || result <= 0) {
        return std::nullopt;
    }
    return result;
}

std::unique_ptr<Element> buildNode(const AstNode& ast, std::size_t depth, BuildResult& result,
                                   const std::string& filename) {
    if (ast.kind == AstNode::Kind::Text) {
        const std::string text = normalizeText(decodeEntities(ast.text));
        if (text.empty()) {
            return nullptr;
        }
        return nullptr;
    }

    auto element = std::make_unique<Element>(lowerCopy(ast.name));
    for (const auto& attribute : ast.attributes) {
        element->attr(lowerCopy(attribute.name), decodeEntities(attribute.value));
    }

    if (depth >= kMaxBuildDepth) {
        result.diagnostics.push_back(makeError("Maximum tree depth exceeded while building DOM",
                                               filename, ast.line, ast.column));
        return element;
    }

    for (const auto& child : ast.children) {
        if (child->kind == AstNode::Kind::Text) {
            const std::string text = normalizeText(decodeEntities(child->text));
            if (text.empty()) {
                continue;
            }
            element->child(std::make_unique<TextNode>(std::move(text)));
            continue;
        }
        auto childElement = buildNode(*child, depth + 1, result, filename);
        if (childElement) {
            element->child(std::move(childElement));
        }
    }
    return element;
}

bool hasError(const std::vector<Diagnostic>& diagnostics) {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& diagnostic) {
        return diagnostic.severity == DiagnosticSeverity::Error;
    });
}

} // namespace

BuildResult buildDocument(const AstDocument& ast) {
    BuildResult result;
    const std::string filename = "<markup>";

    const AstNode* root = nullptr;
    for (const auto& candidate : ast.roots) {
        if (candidate->kind == AstNode::Kind::Element) {
            root = candidate.get();
            break;
        }
    }
    if (root == nullptr) {
        result.diagnostics.push_back(makeError("Markup root must be an element", filename, 1, 1));
        return result;
    }
    if (hasError(result.diagnostics)) {
        return result;
    }

    const AstNode* window = nullptr;
    if (lowerCopy(root->name) == "app") {
        for (const auto& child : root->children) {
            if (child->kind == AstNode::Kind::Element &&
                lowerCopy(child->name) == "window") {
                window = child.get();
                break;
            }
        }
        if (window != nullptr) {
            root = window;
        }
    }
    if (lowerCopy(root->name) == "window") {
        window = root;
    }

    auto document = std::make_unique<Document>();
    auto rootElement = buildNode(*root, 0, result, filename);
    if (!rootElement) {
        result.diagnostics.push_back(makeError("Unable to build the root element", filename,
                                               root->line, root->column));
        return result;
    }

    if (window != nullptr) {
        for (const auto& attribute : window->attributes) {
            const std::string name = lowerCopy(attribute.name);
            if (name == "title") {
                result.title = decodeEntities(attribute.value);
            } else if (name == "width" || name == "height") {
                const auto number = parsePositiveInt(attribute.value);
                if (!number) {
                    result.diagnostics.push_back(makeWarning(
                        "Invalid window " + name + " value '" + attribute.value + "'", filename,
                        attribute.line, attribute.column));
                } else if (name == "width") {
                    result.width = *number;
                } else {
                    result.height = *number;
                }
            }
        }
    }

    document->setRoot(std::move(rootElement));
    result.document = std::move(document);
    return result;
}

} // namespace EntroUI
