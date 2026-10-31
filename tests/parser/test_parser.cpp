#include <gtest/gtest.h>

#include <string>

#include "entroui/entroui.hpp"

using namespace EntroUI;

namespace {

bool hasErrors(const std::vector<Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }
    return false;
}

std::string serialize(const Element& element) {
    std::string result = "<" + element.tag();
    result += " id=\"" + element.id() + "\"";
    for (const auto& [name, value] : element.attributes()) {
        if (name == "id") {
            continue;
        }
        result += " " + name + "=\"" + value + "\"";
    }
    result += ">";
    for (const auto& child : element.children()) {
        if (child->isText()) {
            result += static_cast<const TextNode&>(*child).text();
        } else {
            result += serialize(static_cast<const Element&>(*child));
        }
    }
    result += "</" + element.tag() + ">";
    return result;
}

} // namespace

TEST(Parser, TokenizesBasicMarkup) {
    Lexer lexer("<div class=\"card\">Hello</div>");
    std::vector<Diagnostic> diagnostics;
    const auto tokens = lexer.tokenize(diagnostics);
    EXPECT_TRUE(diagnostics.empty());
    ASSERT_GE(tokens.size(), 6U);
    EXPECT_EQ(tokens[0].type, TokenType::TagOpen);
    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(tokens[1].text, "div");
    EXPECT_EQ(tokens[2].type, TokenType::Identifier);
    EXPECT_EQ(tokens[2].text, "class");
    EXPECT_EQ(tokens[3].type, TokenType::Equals);
    EXPECT_EQ(tokens[4].type, TokenType::String);
    EXPECT_EQ(tokens[4].text, "card");
    EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}

TEST(Parser, ParsesNestedElementsAndAttributes) {
    Parser parser;
    auto result = parser.parse(
        "<div class=\"card\"><h1 id=\"title\">Title</h1><p>Description</p></div>");
    ASSERT_TRUE(result.ok());
    EXPECT_FALSE(hasErrors(result.diagnostics));

    const auto& root = *result->roots.front();
    EXPECT_EQ(root.name, "div");
    ASSERT_EQ(root.attributes.size(), 1U);
    EXPECT_EQ(root.attributes[0].name, "class");
    EXPECT_EQ(root.attributes[0].value, "card");
    ASSERT_EQ(root.children.size(), 2U);
    EXPECT_EQ(root.children[0]->name, "h1");
    EXPECT_EQ(root.children[0]->children[0]->text, "Title");
}

TEST(Parser, HandlesSelfClosingAndVoidTags) {
    Parser parser;
    auto result = parser.parse("<div><image src=\"a.png\" /><input value=\"x\"></div>");
    ASSERT_TRUE(result.ok());
    EXPECT_FALSE(hasErrors(result.diagnostics));
    const auto& root = *result->roots.front();
    EXPECT_EQ(root.children.size(), 2U);
    EXPECT_EQ(root.children[0]->name, "image");
    EXPECT_EQ(root.children[1]->name, "input");
}

TEST(Parser, IgnoresCommentsAndDoctype) {
    Parser parser;
    auto result = parser.parse(
        "<!DOCTYPE html><!-- comment --><div><!-- inner -->text</div>");
    ASSERT_TRUE(result.ok());
    EXPECT_FALSE(hasErrors(result.diagnostics));
    ASSERT_EQ(result->roots.size(), 1U);
    EXPECT_EQ(result->roots[0]->name, "div");
}

TEST(Parser, ReportsMissingClosingTag) {
    Parser parser;
    auto result = parser.parse("<div><p>Hello</div>");
    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(hasErrors(result.diagnostics));
    bool found = false;
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.message.find("Mismatched closing tag") != std::string::npos) {
            found = true;
            EXPECT_GT(diagnostic.line, 0U);
            EXPECT_GT(diagnostic.column, 0U);
        }
    }
    EXPECT_TRUE(found);
}

TEST(Parser, ReportsMultipleErrorsWithLocations) {
    Parser parser;
    auto result = parser.parse("<div>\n  <span>\n</div>");
    EXPECT_TRUE(hasErrors(result.diagnostics));
    for (const auto& diagnostic : result.diagnostics) {
        EXPECT_FALSE(diagnostic.format().empty());
    }
}

TEST(Parser, RejectsSourceOverLimit) {
    Parser parser(ParserOptions{8, 10});
    auto result = parser.parse("<div>hello</div>");
    EXPECT_TRUE(hasErrors(result.diagnostics));
}

TEST(Parser, EnforcesMaximumDepth) {
    Parser parser(ParserOptions{3, 1024 * 1024});
    auto result = parser.parse("<a><b><c><d><e></e></d></c></b></a>");
    EXPECT_TRUE(hasErrors(result.diagnostics));
    bool depthError = false;
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.message.find("Maximum nesting depth") != std::string::npos) {
            depthError = true;
        }
    }
    EXPECT_TRUE(depthError);
}

TEST(Parser, ReportsUnexpectedTextOutsideRoot) {
    Parser parser;
    auto result = parser.parse("hello <div></div>");
    EXPECT_TRUE(hasErrors(result.diagnostics));
}

TEST(Builder, BuildsDocumentAndWindowMetadata) {
    Parser parser;
    auto parsed = parser.parse(R"(
        <app>
          <window title="Demo" width="640" height="480">
            <div class="container">
              <h1>Hello &amp; welcome</h1>
              <p>Line<br/>break</p>
            </div>
          </window>
        </app>
    )");
    ASSERT_TRUE(parsed.ok());
    auto built = buildDocument(*parsed.value);
    ASSERT_TRUE(built.ok());
    EXPECT_EQ(built.title, "Demo");
    EXPECT_EQ(built.width, 640);
    EXPECT_EQ(built.height, 480);
    EXPECT_EQ(built.document->findById("missing"), nullptr);
    auto heading = built.document->findByTag("h1");
    ASSERT_EQ(heading.size(), 1U);
    EXPECT_EQ(heading[0]->textContent(), "Hello & welcome");
}

TEST(Builder, MarkupAndDslProduceEquivalentTrees) {
    Parser parser;
    auto parsed = parser.parse(R"(
        <div class="container">
          <h1>Hello EntroUI</h1>
          <p>Welcome</p>
          <button id="submit">Click Me</button>
        </div>
    )");
    ASSERT_TRUE(parsed.ok());
    auto built = buildDocument(*parsed.value);
    ASSERT_TRUE(built.ok());

    auto dsl = div().className("container");
    dsl.child(h1().text("Hello EntroUI"));
    dsl.child(p().text("Welcome"));
    dsl.child(button("Click Me").id("submit"));

    EXPECT_EQ(serialize(built.document->root()), serialize(dsl));
}

TEST(Builder, NormalizesWhitespaceAndEntities) {
    Parser parser;
    auto parsed = parser.parse("<p>\n   Hello   <span>there</span>  &lt;3\n</p>");
    ASSERT_TRUE(parsed.ok());
    auto built = buildDocument(*parsed.value);
    ASSERT_TRUE(built.ok());
    EXPECT_EQ(built.document->root().textContent(), " Hello there <3 ");
}

TEST(Parser, ReportsUnterminatedStringCommentAndTag) {
    Parser parser;
    auto stringResult = parser.parse("<div title=\"unterminated></div>");
    EXPECT_TRUE(hasErrors(stringResult.diagnostics));

    auto commentResult = parser.parse("<div><!-- missing end</div>");
    EXPECT_TRUE(hasErrors(commentResult.diagnostics));

    auto tagResult = parser.parse("<div class=\"x\"");
    EXPECT_TRUE(hasErrors(tagResult.diagnostics));
}

TEST(Parser, ParseFileReportsMissingFile) {
    Parser parser;
    auto result = parser.parseFile("/tmp/entroui-does-not-exist.eui");
    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(hasErrors(result.diagnostics));
    EXPECT_NE(result.diagnostics.front().format().find("Unable to open"), std::string::npos);
}
