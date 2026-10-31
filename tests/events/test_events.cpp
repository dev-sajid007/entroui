#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "entroui/entroui.hpp"

using namespace EntroUI;

namespace {

class Harness {
public:
    explicit Harness(Element root) {
        document_.setRoot(std::make_unique<Element>(std::move(root)));
        document_.setViewport(500.0F, 400.0F);
        computeDocumentStyles(document_, defaultThemeStylesheet().get(),
                              document_.stylesheet());
        layout_.layout(document_, 500.0F, 400.0F, measurer_);
        dispatcher_ = std::make_unique<EventDispatcher>(document_, layout_, &measurer_);
    }

    [[nodiscard]] Document& document() { return document_; }
    [[nodiscard]] LayoutTree& layout() { return layout_; }
    [[nodiscard]] EventDispatcher& dispatcher() { return *dispatcher_; }
    [[nodiscard]] FixedTextMeasurer& measurer() { return measurer_; }

    [[nodiscard]] Rect box(std::string_view id) const {
        const Element* element = document_.findById(id);
        const LayoutBox* layoutBox = element != nullptr ? layout_.find(element) : nullptr;
        EXPECT_NE(layoutBox, nullptr);
        return layoutBox != nullptr ? layoutBox->borderBox : Rect{};
    }

    void relayout() {
        computeDocumentStyles(document_, defaultThemeStylesheet().get(),
                              document_.stylesheet());
        layout_.layout(document_, 500.0F, 400.0F, measurer_);
    }

private:
    Document document_;
    FixedTextMeasurer measurer_;
    LayoutTree layout_;
    std::unique_ptr<EventDispatcher> dispatcher_;
};

} // namespace

TEST(Events, ClickInvokesCallback) {
    int clicks = 0;
    int lastClicks = 0;
    auto root = div();
    root.child(button("Save")
                   .id("save")
                   .style("width", "80px")
                   .style("height", "40px")
                   .onClick([&](Event& event) {
                       ++clicks;
                       lastClicks = static_cast<MouseEvent&>(event).clicks;
                   }));
    Harness harness(std::move(root));
    harness.dispatcher().handleMouseDown(10.0F, 10.0F, 1);
    harness.dispatcher().handleMouseUp(10.0F, 10.0F, 1);
    EXPECT_EQ(clicks, 1);
    EXPECT_EQ(lastClicks, 1);
}

TEST(Events, ClickBubblesAndCanStopPropagation) {
    std::vector<std::string> order;
    auto root = div().id("root");
    auto child = span().id("child");
    child.style("width", "40px").style("height", "20px");
    child.onClick([&](Event& event) {
        order.push_back("child");
        event.stopPropagation();
    });
    root.onClick([&](Event&) { order.push_back("root"); });
    root.child(std::move(child));
    Harness harness(std::move(root));
    harness.dispatcher().handleMouseDown(5.0F, 5.0F, 1);
    harness.dispatcher().handleMouseUp(5.0F, 5.0F, 1);
    ASSERT_EQ(order.size(), 1U);
    EXPECT_EQ(order[0], "child");
}

TEST(Events, MouseEnterLeaveUpdatesHoverClasses) {
    int enters = 0;
    int leaves = 0;
    auto root = div().id("root");
    auto child = div().id("child");
    child.style("width", "50px").style("height", "50px");
    child.onMouseEnter([&](Event&) { ++enters; });
    child.onMouseLeave([&](Event&) { ++leaves; });
    root.child(std::move(child));
    Harness harness(std::move(root));

    harness.dispatcher().handleMouseMove(10.0F, 10.0F);
    ASSERT_NE(harness.dispatcher().hovered(), nullptr);
    EXPECT_EQ(harness.dispatcher().hovered()->id(), "child");
    EXPECT_TRUE(harness.document().findById("child")->hasClass("hover"));
    EXPECT_TRUE(harness.document().findById("root")->hasClass("hover"));
    EXPECT_EQ(enters, 1);

    harness.dispatcher().handleMouseMove(600.0F, 500.0F);
    EXPECT_EQ(harness.dispatcher().hovered(), nullptr);
    EXPECT_FALSE(harness.document().findById("child")->hasClass("hover"));
    EXPECT_FALSE(harness.document().findById("root")->hasClass("hover"));
    EXPECT_EQ(leaves, 1);
}

TEST(Events, FocusAndTabNavigation) {
    auto root = div();
    root.child(input("Name").id("name"));
    root.child(button("Go").id("go"));
    Harness harness(std::move(root));

    const Rect inputBox = harness.box("name");
    harness.dispatcher().handleMouseDown(inputBox.x + 5.0F, inputBox.y + 5.0F, 1);
    harness.dispatcher().handleMouseUp(inputBox.x + 5.0F, inputBox.y + 5.0F, 1);
    ASSERT_NE(harness.dispatcher().focused(), nullptr);
    EXPECT_EQ(harness.dispatcher().focused()->id(), "name");
    EXPECT_TRUE(harness.document().findById("name")->hasClass("focus"));

    harness.dispatcher().handleKeyDown(Keys::Tab, 0, 0);
    ASSERT_NE(harness.dispatcher().focused(), nullptr);
    EXPECT_EQ(harness.dispatcher().focused()->id(), "go");

    harness.dispatcher().handleKeyDown(Keys::Tab, 0, 1);
    ASSERT_NE(harness.dispatcher().focused(), nullptr);
    EXPECT_EQ(harness.dispatcher().focused()->id(), "name");
}

TEST(Events, TextInputEditing) {
    auto root = div();
    root.child(input("Type").id("input"));
    Harness harness(std::move(root));
    const Rect box = harness.box("input");
    harness.dispatcher().handleMouseDown(box.x + 5.0F, box.y + 5.0F, 1);
    harness.dispatcher().handleMouseUp(box.x + 5.0F, box.y + 5.0F, 1);

    harness.dispatcher().handleTextInput("hello");
    EXPECT_EQ(harness.document().findById("input")->value(), "hello");

    harness.dispatcher().handleKeyDown(Keys::Backspace, 0, 0);
    EXPECT_EQ(harness.document().findById("input")->value(), "hell");

    harness.dispatcher().handleKeyDown(Keys::Left, 0, 0);
    harness.dispatcher().handleKeyDown(Keys::Backspace, 0, 0);
    EXPECT_EQ(harness.document().findById("input")->value(), "hel");
}

TEST(Events, CheckboxTogglesAndEmitsChange) {
    bool changed = false;
    auto checkboxWidget = Widgets::checkbox("Remember me", false);
    checkboxWidget.onChange([&](Event&) { changed = true; });
    auto root = div();
    root.child(std::move(checkboxWidget));
    Harness harness(std::move(root));

    Element* wrapper = harness.document().root().childAt(0) != nullptr
                           ? static_cast<Element*>(harness.document().root().childAt(0))
                           : nullptr;
    ASSERT_NE(wrapper, nullptr);
    Element* box = wrapper->childAt(0) != nullptr
                       ? static_cast<Element*>(wrapper->childAt(0))
                       : nullptr;
    ASSERT_NE(box, nullptr);
    const LayoutBox* layoutBox = harness.layout().find(box);
    ASSERT_NE(layoutBox, nullptr);
    const Rect rect = layoutBox->borderBox;

    harness.dispatcher().handleMouseDown(rect.x + rect.width * 0.5F,
                                         rect.y + rect.height * 0.5F, 1);
    harness.dispatcher().handleMouseUp(rect.x + rect.width * 0.5F,
                                       rect.y + rect.height * 0.5F, 1);
    EXPECT_TRUE(box->checked());
    EXPECT_TRUE(changed);
}

TEST(Events, KeyboardActivatesFocusedButton) {
    int clicks = 0;
    auto root = div();
    root.child(button("Go").id("go").onClick([&](Event&) { ++clicks; }));
    Harness harness(std::move(root));
    harness.dispatcher().setFocus(harness.document().findById("go"));
    harness.dispatcher().handleKeyDown(Keys::Enter, 0, 0);
    EXPECT_EQ(clicks, 1);
}

TEST(Events, WheelScrollsContainer) {
    auto root = div();
    auto scroll = Widgets::scrollView(div().child(div().style("height", "300px")), 100.0F);
    scroll.id("scroll");
    root.child(std::move(scroll));
    Harness harness(std::move(root));

    const Rect box = harness.box("scroll");
    harness.dispatcher().handleWheel(box.x + 5.0F, box.y + 5.0F, 0.0F, 40.0F);
    Element* element = harness.document().findById("scroll");
    ASSERT_NE(element, nullptr);
    EXPECT_FLOAT_EQ(element->scrollY(), 40.0F);

    harness.dispatcher().handleWheel(box.x + 5.0F, box.y + 5.0F, 0.0F, 1000.0F);
    EXPECT_LE(element->scrollY(), 220.0F);
}

TEST(Events, HoverClassesDriveThemeStyling) {
    auto root = div();
    root.child(button("Hover").id("button"));
    Harness harness(std::move(root));
    const Rect box = harness.box("button");
    harness.dispatcher().handleMouseMove(box.x + 2.0F, box.y + 2.0F);
    ASSERT_NE(harness.dispatcher().hovered(), nullptr);
    EXPECT_TRUE(harness.dispatcher().hovered()->hasClass("hover"));
}

TEST(Events, ClickPlacesCaretAtMeasuredPosition) {
    auto root = div();
    auto field = input("Type").id("input");
    field.setValue("hello");
    root.child(std::move(field));
    Harness harness(std::move(root));

    Element* element = harness.document().findById("input");
    ASSERT_NE(element, nullptr);
    const LayoutBox* box = harness.layout().find(element);
    ASSERT_NE(box, nullptr);
    const float clickX = box->contentBox.x + 16.0F;
    harness.dispatcher().handleMouseDown(clickX, box->contentBox.y + 5.0F, 1);
    harness.dispatcher().handleMouseUp(clickX, box->contentBox.y + 5.0F, 1);
    EXPECT_EQ(element->caret(), 2);
}

TEST(Events, ClipboardCopyPasteAndCut) {
    auto root = div();
    root.child(input("Type").id("input"));
    Harness harness(std::move(root));
    Element* element = harness.document().findById("input");
    ASSERT_NE(element, nullptr);
    element->setValue("hello");
    harness.dispatcher().setFocus(element);

    std::string clipboard;
    harness.dispatcher().setClipboard([&] { return clipboard; },
                                      [&](std::string value) { clipboard = std::move(value); });

    harness.dispatcher().handleKeyDown('a', 0, Mods::Ctrl);
    EXPECT_EQ(element->selectedText(), "hello");
    harness.dispatcher().handleKeyDown('c', 0, Mods::Ctrl);
    EXPECT_EQ(clipboard, "hello");

    clipboard = "world";
    harness.dispatcher().handleKeyDown('v', 0, Mods::Ctrl);
    EXPECT_EQ(element->value(), "world");
    EXPECT_EQ(element->caret(), 5);

    harness.dispatcher().handleKeyDown('a', 0, Mods::Ctrl);
    harness.dispatcher().handleKeyDown('x', 0, Mods::Ctrl);
    EXPECT_EQ(element->value(), "");
    EXPECT_EQ(clipboard, "world");
}

TEST(Events, ShiftSelectionAndBackspace) {
    auto root = div();
    root.child(input("Type").id("input"));
    Harness harness(std::move(root));
    Element* element = harness.document().findById("input");
    ASSERT_NE(element, nullptr);
    element->setValue("abc");
    harness.dispatcher().setFocus(element);

    harness.dispatcher().handleKeyDown(Keys::Left, 0, Mods::Shift);
    harness.dispatcher().handleKeyDown(Keys::Left, 0, Mods::Shift);
    EXPECT_EQ(element->selectedText(), "bc");
    harness.dispatcher().handleKeyDown(Keys::Backspace, 0, 0);
    EXPECT_EQ(element->value(), "a");
    EXPECT_FALSE(element->hasSelection());
}

TEST(Events, PasteReplacesSelection) {
    auto root = div();
    root.child(input("Type").id("input"));
    Harness harness(std::move(root));
    Element* element = harness.document().findById("input");
    ASSERT_NE(element, nullptr);
    element->setValue("abcdef");
    harness.dispatcher().setFocus(element);
    harness.dispatcher().setClipboard([] { return std::string("XY"); }, [](std::string) {});

    harness.dispatcher().handleKeyDown(Keys::Home, 0, 0);
    harness.dispatcher().handleKeyDown(Keys::Right, 0, Mods::Shift);
    harness.dispatcher().handleKeyDown(Keys::Right, 0, Mods::Shift);
    EXPECT_EQ(element->selectedText(), "ab");
    harness.dispatcher().handleKeyDown('v', 0, Mods::Ctrl);
    EXPECT_EQ(element->value(), "XYcdef");
}
