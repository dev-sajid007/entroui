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

int warningCount(const std::vector<Diagnostic>& diagnostics) {
    int count = 0;
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::Warning) {
            ++count;
        }
    }
    return count;
}

StyleSheet parseSheet(const std::string& css) {
    auto parsed = StyleSheet::parse(css);
    EXPECT_FALSE(hasErrors(parsed.diagnostics));
    EXPECT_TRUE(parsed.ok());
    return parsed.ok() ? std::move(*parsed.value) : StyleSheet{};
}

void styleDocument(Document& document, const StyleSheet* user = nullptr) {
    computeDocumentStyles(document, defaultThemeStylesheet().get(), user);
}

} // namespace

TEST(Style, ParsesColors) {
    EXPECT_EQ(Color::parse("#fff"), Color(255, 255, 255, 255));
    EXPECT_EQ(Color::parse("#2563eb"), Color(37, 99, 235, 255));
    EXPECT_EQ(Color::parse("#00000080"), Color(0, 0, 0, 128));
    EXPECT_EQ(Color::parse("red"), Color(255, 0, 0, 255));
    EXPECT_EQ(Color::parse("rgb(10, 20, 30)"), Color(10, 20, 30, 255));
    EXPECT_EQ(Color::parse("rgba(10,20,30,0.5)").a, 128);
    EXPECT_FALSE(Color::tryParse("nonsense").has_value());
    EXPECT_FALSE(Color::tryParse("#12").has_value());
    EXPECT_THROW(Color::parse("bad"), std::invalid_argument);
}

TEST(Style, ParsesLengths) {
    EXPECT_EQ(Length::parse("10px").resolve(100.0F), 10.0F);
    EXPECT_EQ(Length::parse("25%").resolve(200.0F), 50.0F);
    EXPECT_TRUE(Length::parse("auto").isAuto());
    EXPECT_EQ(Length::parse("0").value, 0.0F);
    EXPECT_FALSE(Length::tryParse("12rem").has_value());
}

TEST(Style, AppliesDeclarations) {
    StyleProperties properties;
    EXPECT_TRUE(applyStyleDeclaration(properties, "width", "120px"));
    EXPECT_EQ(properties.width.resolve(300.0F), 120.0F);
    EXPECT_TRUE(applyStyleDeclaration(properties, "padding", "10px 20px"));
    EXPECT_EQ(properties.padding.top.value, 10.0F);
    EXPECT_EQ(properties.padding.right.value, 20.0F);
    EXPECT_TRUE(applyStyleDeclaration(properties, "border", "2px solid #112233"));
    EXPECT_EQ(properties.borderWidth, 2.0F);
    EXPECT_EQ(properties.borderColor, Color(17, 34, 51, 255));
    EXPECT_TRUE(applyStyleDeclaration(properties, "font-weight", "bold"));
    EXPECT_EQ(properties.fontWeight, 700);
    EXPECT_TRUE(applyStyleDeclaration(properties, "display", "flex"));
    EXPECT_EQ(properties.display, Display::Flex);
    EXPECT_TRUE(applyStyleDeclaration(properties, "justify-content", "space-between"));
    EXPECT_EQ(properties.justifyContent, JustifyContent::SpaceBetween);
    EXPECT_FALSE(applyStyleDeclaration(properties, "width", "banana"));
    EXPECT_FALSE(applyStyleDeclaration(properties, "unknown", "1"));
}

TEST(Style, ParsesSelectorsAndSpecificity) {
    auto tag = parseSelector("button");
    ASSERT_TRUE(tag.ok());
    EXPECT_EQ(tag->specificity(), 1);

    auto classSelector = parseSelector(".primary");
    ASSERT_TRUE(classSelector.ok());
    EXPECT_EQ(classSelector->specificity(), 10);

    auto compound = parseSelector("button.primary.large");
    ASSERT_TRUE(compound.ok());
    EXPECT_EQ(compound->specificity(), 21);

    auto idSelector = parseSelector("#submit");
    ASSERT_TRUE(idSelector.ok());
    EXPECT_EQ(idSelector->specificity(), 100);

    EXPECT_FALSE(parseSelector("div > p").ok());
    EXPECT_FALSE(parseSelector("").ok());
    EXPECT_FALSE(parseSelector(":hover").ok());
}

TEST(Style, ParsesStylesheetAndCascade) {
    const auto sheet = parseSheet(R"(
        /* comments are allowed */
        div { color: #111111; background: #eeeeee; }
        .card { background: #ff0000; padding: 10px; }
        #special { background: #00ff00; }
        .card, .panel { border: 1px solid #333; }
    )");

    Document document;
    auto root = div().className("card").id("special").child(h1().text("Title"));
    document.setRoot(std::make_unique<Element>(std::move(root)));
    styleDocument(document, &sheet);

    const auto& style = document.root().computedStyle();
    EXPECT_EQ(style.background, Color(0, 255, 0, 255));
    EXPECT_EQ(style.color, Color(17, 17, 17, 255));
    EXPECT_EQ(style.padding.top.value, 10.0F);
    EXPECT_EQ(style.borderWidth, 1.0F);
    EXPECT_EQ(style.display, Display::Block);
}

TEST(Style, InheritanceAndHeadingDefaults) {
    const auto sheet = parseSheet(R"(
        .body { color: #abcdef; font-size: 20px; }
        .muted { color: #222222; }
    )");

    Document document;
    auto root = div().className("body");
    auto heading = h1().text("Hello");
    auto paragraph = p().className("muted").text("Text");
    root.child(std::move(heading));
    root.child(std::move(paragraph));
    document.setRoot(std::make_unique<Element>(std::move(root)));
    styleDocument(document, &sheet);

    const auto& headingStyle =
        static_cast<Element&>(*document.root().childAt(0)).computedStyle();
    EXPECT_EQ(headingStyle.fontSize, 32.0F);
    EXPECT_EQ(headingStyle.color, Color(171, 205, 239, 255));
    EXPECT_EQ(headingStyle.fontWeight, 700);

    const auto& paragraphStyle =
        static_cast<Element&>(*document.root().childAt(1)).computedStyle();
    EXPECT_EQ(paragraphStyle.fontSize, 20.0F);
    EXPECT_EQ(paragraphStyle.color, Color(34, 34, 34, 255));
}

TEST(Style, InlineStylesWin) {
    const auto sheet = parseSheet(".box { width: 100px; background: red; }");
    auto element = div().className("box");
    element.style("width", "250px");
    element.style("background", "#00ff00");

    Document document;
    document.setRoot(std::make_unique<Element>(std::move(element)));
    styleDocument(document, &sheet);
    EXPECT_EQ(document.root().computedStyle().width.resolve(1000.0F), 250.0F);
    EXPECT_EQ(document.root().computedStyle().background, Color(0, 255, 0, 255));
}

TEST(Style, ThemeProvidesControlDefaults) {
    Document document;
    document.setRoot(std::make_unique<Element>(button("Save")));
    styleDocument(document);
    const auto& style = document.root().computedStyle();
    EXPECT_EQ(style.background, Color(37, 99, 235, 255));
    EXPECT_EQ(style.color, Color(255, 255, 255, 255));
    EXPECT_EQ(style.padding.top.value, 10.0F);
    EXPECT_EQ(style.borderRadius, 6.0F);
}

TEST(Style, ReportsInvalidDeclarationsButKeepsRule) {
    auto parsed = StyleSheet::parse(
        ".x { mystery: 1; width: banana; height: 20px; }");
    EXPECT_TRUE(parsed.ok());
    EXPECT_FALSE(hasErrors(parsed.diagnostics));
    EXPECT_EQ(warningCount(parsed.diagnostics), 2);

    Document document;
    document.setRoot(std::make_unique<Element>(div().className("x")));
    styleDocument(document, &*parsed.value);
    EXPECT_EQ(document.root().computedStyle().height.resolve(100.0F), 20.0F);
}

TEST(Style, ClassChangesInvalidateStyles) {
    Document document;
    auto root = div().className("a");
    document.setRoot(std::make_unique<Element>(std::move(root)));
    const auto sheet = parseSheet(".a { background: red; } .b { background: blue; }");
    styleDocument(document, &sheet);
    EXPECT_EQ(document.root().computedStyle().background, Color(255, 0, 0, 255));
    document.root().addClass("b");
    styleDocument(document, &sheet);
    EXPECT_EQ(document.root().computedStyle().background, Color(0, 0, 255, 255));
}
