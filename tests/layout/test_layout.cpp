#include <gtest/gtest.h>

#include <string>

#include "entroui/entroui.hpp"

using namespace EntroUI;

namespace {

StyleSheet sheetFor(const std::string& css) {
    auto parsed = StyleSheet::parse(css);
    EXPECT_TRUE(parsed.ok());
    return parsed.ok() ? std::move(*parsed.value) : StyleSheet{};
}

void prepare(Document& document, LayoutTree& layout, TextMeasurer& measurer,
             const StyleSheet* user = nullptr, float width = 500.0F, float height = 300.0F) {
    document.setViewport(width, height);
    computeDocumentStyles(document, defaultThemeStylesheet().get(), user);
    layout.layout(document, width, height, measurer);
}

const LayoutBox* boxFor(const LayoutTree& layout, const Document& document,
                        std::string_view id) {
    const Element* element = document.findById(id);
    return element != nullptr ? layout.find(element) : nullptr;
}

} // namespace

TEST(Layout, BlockBoxModel) {
    Document document;
    auto root = div();
    auto child = div().id("child");
    child.style("width", "100px").style("height", "50px").style("margin", "10px").style(
        "padding", "5px");
    root.child(std::move(child));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    const LayoutBox* box = boxFor(layout, document, "child");
    ASSERT_NE(box, nullptr);
    EXPECT_FLOAT_EQ(box->borderBox.x, 10.0F);
    EXPECT_FLOAT_EQ(box->borderBox.y, 10.0F);
    EXPECT_FLOAT_EQ(box->borderBox.width, 100.0F);
    EXPECT_FLOAT_EQ(box->borderBox.height, 50.0F);
    EXPECT_FLOAT_EQ(box->contentBox.width, 90.0F);
    EXPECT_FLOAT_EQ(box->contentBox.height, 40.0F);
}

TEST(Layout, PercentAndAutoWidths) {
    Document document;
    auto root = div();
    auto percent = div().id("percent");
    percent.style("width", "50%").style("height", "20px");
    auto automatic = div().id("automatic");
    automatic.style("height", "20px");
    root.child(std::move(percent));
    root.child(std::move(automatic));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer, nullptr, 400.0F, 300.0F);

    EXPECT_FLOAT_EQ(boxFor(layout, document, "percent")->borderBox.width, 200.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "automatic")->borderBox.width, 400.0F);
}

TEST(Layout, FlexRowJustifyCenterAlignCenterGap) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex")
        .style("flex-direction", "row")
        .style("justify-content", "center")
        .style("align-items", "center")
        .style("gap", "10px")
        .style("width", "300px")
        .style("height", "100px")
        .style("padding", "10px");
    auto first = div().id("first");
    first.style("width", "50px").style("height", "20px");
    auto second = div().id("second");
    second.style("width", "50px").style("height", "20px");
    root.child(std::move(first));
    root.child(std::move(second));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    const LayoutBox* firstBox = boxFor(layout, document, "first");
    const LayoutBox* secondBox = boxFor(layout, document, "second");
    ASSERT_NE(firstBox, nullptr);
    ASSERT_NE(secondBox, nullptr);
    EXPECT_FLOAT_EQ(firstBox->borderBox.x, 95.0F);
    EXPECT_FLOAT_EQ(firstBox->borderBox.y, 40.0F);
    EXPECT_FLOAT_EQ(secondBox->borderBox.x, 155.0F);
    EXPECT_FLOAT_EQ(secondBox->borderBox.y, 40.0F);
}

TEST(Layout, FlexColumnJustifyEnd) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex")
        .style("flex-direction", "column")
        .style("justify-content", "flex-end")
        .style("gap", "10px")
        .style("width", "200px")
        .style("height", "200px");
    auto first = div().id("first");
    first.style("width", "40px").style("height", "50px");
    auto second = div().id("second");
    second.style("width", "40px").style("height", "50px");
    root.child(std::move(first));
    root.child(std::move(second));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    EXPECT_FLOAT_EQ(boxFor(layout, document, "first")->borderBox.y, 90.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "second")->borderBox.y, 150.0F);
}

TEST(Layout, FlexStretchCrossAxis) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex")
        .style("align-items", "stretch")
        .style("width", "300px")
        .style("height", "120px")
        .style("padding", "10px");
    auto child = div().id("child");
    child.style("width", "80px");
    root.child(std::move(child));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    const LayoutBox* box = boxFor(layout, document, "child");
    ASSERT_NE(box, nullptr);
    EXPECT_FLOAT_EQ(box->borderBox.height, 100.0F);
}

TEST(Layout, TextWrappingAndLineHeight) {
    Document document;
    auto root = div();
    auto paragraph = p().id("text");
    paragraph.style("width", "50px").style("font-size", "10px").style("line-height", "12px");
    paragraph.text("hello world again");
    root.child(std::move(paragraph));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    const LayoutBox* box = boxFor(layout, document, "text");
    ASSERT_NE(box, nullptr);
    ASSERT_EQ(box->children.size(), 3U);
    float totalHeight = 0.0F;
    std::vector<std::string> lines;
    for (const auto& child : box->children) {
        ASSERT_TRUE(child->isText);
        ASSERT_EQ(child->lines.size(), 1U);
        lines.push_back(child->lines[0]);
        totalHeight += child->contentBox.height;
    }
    EXPECT_EQ(lines[0], "hello");
    EXPECT_EQ(lines[1], "world");
    EXPECT_EQ(lines[2], "again");
    EXPECT_FLOAT_EQ(totalHeight, 36.0F);
}

TEST(Layout, TextAlignDoesNotChangeGeometry) {
    Document document;
    auto root = div();
    auto paragraph = p().id("text");
    paragraph.style("width", "100px").style("text-align", "center");
    paragraph.text("hi");
    root.child(std::move(paragraph));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);
    const auto& textBox = *boxFor(layout, document, "text")->children[0];
    EXPECT_EQ(textBox.textAlign, TextAlign::Center);
}

TEST(Layout, DisplayNoneIsSkipped) {
    Document document;
    auto root = div();
    auto hidden = div().id("hidden");
    hidden.style("display", "none").style("height", "50px");
    auto visible = div().id("visible");
    visible.style("height", "20px");
    root.child(std::move(hidden));
    root.child(std::move(visible));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    EXPECT_EQ(layout.find(document.findById("hidden")), nullptr);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "visible")->borderBox.y, 0.0F);
}

TEST(Layout, MinMaxConstraints) {
    Document document;
    auto root = div();
    auto child = div().id("child");
    child.style("width", "50px").style("min-width", "120px").style("max-height", "30px");
    child.style("height", "80px");
    root.child(std::move(child));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);
    const LayoutBox* box = boxFor(layout, document, "child");
    EXPECT_FLOAT_EQ(box->borderBox.width, 120.0F);
    EXPECT_FLOAT_EQ(box->borderBox.height, 30.0F);
}

TEST(Layout, ScrollContentAndClipping) {
    Document document;
    auto root = div().id("scroll");
    root.style("height", "100px").style("width", "200px");
    root.setScrollable(true);
    root.setScroll(0.0F, 30.0F);
    auto first = div().id("first");
    first.style("height", "60px");
    auto second = div().id("second");
    second.style("height", "60px");
    root.child(std::move(first));
    root.child(std::move(second));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    const LayoutBox* scroll = boxFor(layout, document, "scroll");
    ASSERT_NE(scroll, nullptr);
    EXPECT_TRUE(scroll->scrollable);
    EXPECT_TRUE(scroll->clip);
    EXPECT_FLOAT_EQ(scroll->scrollContentHeight, 120.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "first")->borderBox.y, -20.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "second")->borderBox.y, 40.0F);
}

TEST(Layout, HitTestingReturnsDeepestElement) {
    Document document;
    auto root = div().id("root");
    root.style("width", "300px").style("height", "200px");
    auto child = div().id("child");
    child.style("width", "100px").style("height", "100px").style("padding", "10px");
    auto grandchild = span().id("grandchild");
    grandchild.style("width", "20px").style("height", "20px");
    child.child(std::move(grandchild));
    root.child(std::move(child));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    const LayoutBox* hit = layout.hitTest(15.0F, 15.0F);
    ASSERT_NE(hit, nullptr);
    ASSERT_NE(hit->element, nullptr);
    EXPECT_EQ(hit->element->id(), "grandchild");

    hit = layout.hitTest(150.0F, 150.0F);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->element->id(), "root");

    EXPECT_EQ(layout.hitTest(900.0F, 900.0F), nullptr);
}

TEST(Layout, InlineBlockUsesIntrinsicWidth) {
    Document document;
    auto root = div();
    auto buttonElement = button("OK");
    buttonElement.id("button");
    root.child(std::move(buttonElement));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);
    const LayoutBox* box = boxFor(layout, document, "button");
    ASSERT_NE(box, nullptr);
    EXPECT_LT(box->borderBox.width, 400.0F);
    EXPECT_GT(box->borderBox.width, 30.0F);
}

TEST(Layout, InlineTextAndSpanShareLine) {
    const auto sheet = sheetFor(".accent { color: #ff0000; font-weight: bold; }");

    Document document;
    auto root = div();
    auto paragraph = p().id("p");
    paragraph.style("width", "400px");
    paragraph.child(textNode("Hello "));
    paragraph.child(span().id("accent").className("accent").text("world"));
    paragraph.child(textNode(" again"));
    root.child(std::move(paragraph));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer, &sheet);

    const LayoutBox* box = boxFor(layout, document, "p");
    ASSERT_NE(box, nullptr);
    ASSERT_GE(box->children.size(), 3U);
    const LayoutBox* accent = boxFor(layout, document, "accent");
    ASSERT_NE(accent, nullptr);
    EXPECT_GT(accent->borderBox.x, box->contentBox.x);
    EXPECT_FLOAT_EQ(accent->borderBox.y, box->contentBox.y);
    bool foundBefore = false;
    bool foundAfter = false;
    for (const auto& child : box->children) {
        if (child->isText) {
            if (child->borderBox.x < accent->borderBox.x) {
                foundBefore = true;
            }
            if (child->borderBox.x > accent->borderBox.x) {
                foundAfter = true;
            }
        }
    }
    EXPECT_TRUE(foundBefore);
    EXPECT_TRUE(foundAfter);
    EXPECT_EQ(accent->element->computedStyle().color, Color(255, 0, 0, 255));
}

TEST(Layout, InlineSpanWrapsToNextLine) {
    Document document;
    auto root = div();
    auto paragraph = p().id("p");
    paragraph.style("width", "60px");
    paragraph.child(textNode("aaaa "));
    paragraph.child(span().id("accent").text("bbbb"));
    root.child(std::move(paragraph));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    const LayoutBox* accent = boxFor(layout, document, "accent");
    ASSERT_NE(accent, nullptr);
    EXPECT_GT(accent->borderBox.y, boxFor(layout, document, "p")->contentBox.y);
}

TEST(Layout, FlexGrowDistributesFreeSpace) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex").style("width", "300px").style("height", "100px");
    auto flexible = div().id("flexible");
    flexible.style("width", "100px").style("height", "20px").style("flex-grow", "1");
    auto fixed = div().id("fixed");
    fixed.style("width", "100px").style("height", "20px");
    root.child(std::move(flexible));
    root.child(std::move(fixed));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "flexible")->borderBox.width, 200.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "fixed")->borderBox.width, 100.0F);
}

TEST(Layout, FlexShrinkPreventsOverflow) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex").style("width", "150px").style("height", "100px");
    auto first = div().id("first");
    first.style("width", "100px").style("height", "20px");
    auto second = div().id("second");
    second.style("width", "100px").style("height", "20px");
    root.child(std::move(first));
    root.child(std::move(second));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "first")->borderBox.width, 75.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "second")->borderBox.width, 75.0F);
}

TEST(Layout, FlexBasisAndShorthand) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex").style("width", "300px").style("height", "100px");
    auto first = div().id("first");
    first.style("flex", "1").style("height", "20px");
    auto second = div().id("second");
    second.style("flex", "1").style("height", "20px");
    root.child(std::move(first));
    root.child(std::move(second));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "first")->borderBox.width, 150.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "second")->borderBox.width, 150.0F);
}

TEST(Layout, MarkupInlineWhitespaceIsPreserved) {
    Parser parser;
    auto parsed = parser.parse("<p><span>A</span> <span>B</span></p>");
    ASSERT_TRUE(parsed.ok());
    auto built = buildDocument(*parsed.value);
    ASSERT_TRUE(built.ok());
    EXPECT_EQ(built.document->root().textContent(), "A B");
}

TEST(Layout, FlexWrapMovesItemsToNextLine) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex")
        .style("flex-wrap", "wrap")
        .style("align-content", "start")
        .style("gap", "10px")
        .style("width", "120px")
        .style("height", "200px");
    for (int index = 0; index < 3; ++index) {
        auto item = div().id("item" + std::to_string(index));
        item.style("width", "50px").style("height", "30px");
        root.child(std::move(item));
    }
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);

    EXPECT_FLOAT_EQ(boxFor(layout, document, "item0")->borderBox.y, 0.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "item1")->borderBox.y, 0.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "item2")->borderBox.y, 40.0F);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "item2")->borderBox.x, 0.0F);
}

TEST(Layout, FlexAlignSelfOverridesAlignItems) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex")
        .style("align-items", "start")
        .style("width", "200px")
        .style("height", "100px");
    auto child = div().id("child");
    child.style("width", "20px").style("height", "20px").style("align-self", "end");
    root.child(std::move(child));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "child")->borderBox.y, 80.0F);
}

TEST(Layout, FlexColumnGapUsesColumnGap) {
    Document document;
    auto root = div().id("root");
    root.style("display", "flex")
        .style("column-gap", "20px")
        .style("row-gap", "40px")
        .style("width", "200px")
        .style("height", "100px");
    auto first = div().id("first");
    first.style("width", "20px").style("height", "20px");
    auto second = div().id("second");
    second.style("width", "20px").style("height", "20px");
    root.child(std::move(first));
    root.child(std::move(second));
    document.setRoot(std::make_unique<Element>(std::move(root)));

    LayoutTree layout;
    FixedTextMeasurer measurer;
    prepare(document, layout, measurer);
    EXPECT_FLOAT_EQ(boxFor(layout, document, "second")->borderBox.x, 40.0F);
}
