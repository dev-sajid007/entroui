#include "entroui/parser/parser.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "entroui/parser/lexer.hpp"
#include "entroui/parser/token.hpp"

namespace EntroUI {

namespace {

bool isVoidTag(std::string_view tag) { return tag == "input" || tag == "image"; }

std::string describe(const Token& token) {
    if (token.type == TokenType::EndOfFile) {
        return "end of file";
    }
    std::string result = tokenTypeName(token.type);
    if (!token.text.empty()) {
        result += " '" + token.text + "'";
    }
    return result;
}

} // namespace

std::string Diagnostic::format() const {
    std::ostringstream stream;
    stream << (file.empty() ? "<memory>" : file) << ':' << line << ':' << column << ": "
           << (severity == DiagnosticSeverity::Error ? "error" : "warning") << ": " << message;
    return stream.str();
}

Parser::Parser(ParserOptions options) : options_(options) {}

ParseResult<AstDocument> Parser::parse(std::string_view source, std::string filename) const {
    ParseResult<AstDocument> result;
    AstDocument document;

    Lexer lexer(source, filename, LexerOptions{options_.maxSourceSize});
    std::vector<Diagnostic> lexicalDiagnostics;
    const std::vector<Token> tokens = lexer.tokenize(lexicalDiagnostics);
    for (auto& diagnostic : lexicalDiagnostics) {
        result.diagnostics.push_back(std::move(diagnostic));
    }

    std::vector<AstNode*> stack;
    std::size_t index = 0;

    auto appendNode = [&](std::unique_ptr<AstNode> node) {
        if (stack.empty()) {
            document.roots.push_back(std::move(node));
        } else {
            stack.back()->children.push_back(std::move(node));
        }
    };

    while (index < tokens.size()) {
        const Token& token = tokens[index];

        if (token.type == TokenType::EndOfFile) {
            break;
        }
        if (token.type == TokenType::Comment) {
            ++index;
            continue;
        }
        if (token.type == TokenType::Text) {
            if (stack.empty()) {
                const bool onlyWhitespace =
                    token.text.find_first_not_of(" \t\r\n") == std::string::npos;
                if (!onlyWhitespace) {
                    result.diagnostics.push_back(makeError(
                        "Text is not allowed outside the root element", filename, token.line,
                        token.column));
                }
                ++index;
                continue;
            }
            auto node = std::make_unique<AstNode>();
            node->kind = AstNode::Kind::Text;
            node->text = token.text;
            node->line = token.line;
            node->column = token.column;
            stack.back()->children.push_back(std::move(node));
            ++index;
            continue;
        }
        if (token.type != TokenType::TagOpen) {
            result.diagnostics.push_back(makeError("Unexpected token " + describe(token), filename,
                                                   token.line, token.column));
            ++index;
            continue;
        }

        const std::size_t openIndex = index;
        ++index;
        if (index >= tokens.size()) {
            result.diagnostics.push_back(makeError("Unexpected end of input after '<'", filename,
                                                   token.line, token.column));
            break;
        }

        if (tokens[index].type == TokenType::Slash) {
            ++index;
            if (index >= tokens.size() || tokens[index].type != TokenType::Identifier) {
                result.diagnostics.push_back(makeError("Expected a tag name after '</'", filename,
                                                       token.line, token.column));
                while (index < tokens.size() && tokens[index].type != TokenType::TagClose &&
                       tokens[index].type != TokenType::EndOfFile) {
                    ++index;
                }
                if (index < tokens.size() && tokens[index].type == TokenType::TagClose) {
                    ++index;
                }
                continue;
            }
            const Token& nameToken = tokens[index];
            ++index;
            if (index >= tokens.size() || tokens[index].type != TokenType::TagClose) {
                result.diagnostics.push_back(makeError("Expected '>' after closing tag", filename,
                                                       nameToken.line, nameToken.column));
                continue;
            }
            ++index;
            if (stack.empty()) {
                result.diagnostics.push_back(makeError(
                    "Unexpected closing tag '</" + nameToken.text + ">'", filename,
                    nameToken.line, nameToken.column));
            } else if (stack.back()->name != nameToken.text) {
                result.diagnostics.push_back(makeError(
                    "Mismatched closing tag '</" + nameToken.text + ">', expected '</" +
                        stack.back()->name + ">'",
                    filename, nameToken.line, nameToken.column));
                stack.pop_back();
            } else {
                stack.pop_back();
            }
            continue;
        }

        if (tokens[index].type != TokenType::Identifier) {
            result.diagnostics.push_back(makeError(
                "Expected a tag name after '<', found " + describe(tokens[index]), filename,
                tokens[index].line, tokens[index].column));
            ++index;
            continue;
        }

        auto node = std::make_unique<AstNode>();
        node->kind = AstNode::Kind::Element;
        node->name = tokens[index].text;
        node->line = tokens[index].line;
        node->column = tokens[index].column;
        ++index;

        bool selfClosing = false;
        bool closed = false;
        while (index < tokens.size()) {
            const Token& attributeToken = tokens[index];
            if (attributeToken.type == TokenType::TagClose) {
                ++index;
                closed = true;
                break;
            }
            if (attributeToken.type == TokenType::Slash) {
                ++index;
                if (index < tokens.size() && tokens[index].type == TokenType::TagClose) {
                    ++index;
                    selfClosing = true;
                    closed = true;
                } else {
                    result.diagnostics.push_back(makeError(
                        "Expected '>' after '/' in self-closing tag", filename,
                        attributeToken.line, attributeToken.column));
                }
                break;
            }
            if (attributeToken.type != TokenType::Identifier) {
                result.diagnostics.push_back(makeError(
                    "Expected an attribute name, found " + describe(attributeToken), filename,
                    attributeToken.line, attributeToken.column));
                ++index;
                continue;
            }

            AstAttribute attribute;
            attribute.name = attributeToken.text;
            attribute.line = attributeToken.line;
            attribute.column = attributeToken.column;
            ++index;
            if (index < tokens.size() && tokens[index].type == TokenType::Equals) {
                ++index;
                if (index < tokens.size() &&
                    (tokens[index].type == TokenType::String ||
                     tokens[index].type == TokenType::Identifier)) {
                    attribute.value = tokens[index].text;
                    ++index;
                } else {
                    result.diagnostics.push_back(makeError(
                        "Expected an attribute value after '='", filename, attributeToken.line,
                        attributeToken.column));
                }
            } else {
                attribute.value = "true";
            }
            for (const auto& existing : node->attributes) {
                if (existing.name == attribute.name) {
                    result.diagnostics.push_back(makeWarning(
                        "Duplicate attribute '" + attribute.name + "'", filename,
                        attributeToken.line, attributeToken.column));
                }
            }
            node->attributes.push_back(std::move(attribute));
        }

        if (!closed) {
            result.diagnostics.push_back(makeError(
                "Unterminated opening tag '<" + node->name + ">'", filename, node->line,
                node->column));
        }

        const bool voidTag = isVoidTag(node->name);
        const bool push = closed && !selfClosing && !voidTag;
        if (push && stack.size() >= options_.maxDepth) {
            result.diagnostics.push_back(makeError(
                "Maximum nesting depth of " + std::to_string(options_.maxDepth) + " exceeded",
                filename, node->line, node->column));
        }

        AstNode* raw = node.get();
        appendNode(std::move(node));
        if (push) {
            stack.push_back(raw);
        }
        if (openIndex >= tokens.size()) {
            break;
        }
    }

    while (!stack.empty()) {
        AstNode* node = stack.back();
        result.diagnostics.push_back(makeError("Missing closing tag for '<" + node->name + ">'",
                                               filename, node->line, node->column));
        stack.pop_back();
    }

    if (document.roots.empty()) {
        result.diagnostics.push_back(
            makeError("Markup contains no root element", filename, 1, 1));
    } else if (document.roots.size() > 1) {
        for (std::size_t rootIndex = 1; rootIndex < document.roots.size(); ++rootIndex) {
            result.diagnostics.push_back(makeError(
                "Multiple root elements are not supported", filename,
                document.roots[rootIndex]->line, document.roots[rootIndex]->column));
        }
    }

    result.value = std::move(document);
    return result;
}

ParseResult<AstDocument> Parser::parseFile(const std::filesystem::path& path) const {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        ParseResult<AstDocument> result;
        result.diagnostics.push_back(makeError("Unable to open markup file", path.string(), 1, 1));
        return result;
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return parse(buffer.str(), path.string());
}

} // namespace EntroUI
