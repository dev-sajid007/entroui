#include <gtest/gtest.h>

#include "entroui/entroui.hpp"

using namespace EntroUI;

TEST(Core, ElementFactoriesAndAttributes) {
    auto element = div().id("root").className("container main").attr("data-x", "1");
    EXPECT_EQ(element.tag(), "div");
    EXPECT_EQ(element.id(), "root");
    EXPECT_TRUE(element.hasClass("container"));
    EXPECT_TRUE(element.hasClass("main"));
    EXPECT_EQ(element.attribute("data-x"), "1");
    EXPECT_EQ(element.classes().size(), 2U);
}

TEST(Core, TreeOwnershipAndParentLinks) {
    auto root = div().child(h1().text("Hello")).child(p().text("World"));
    EXPECT_EQ(root.childCount(), 2U);
    auto* first = static_cast<Element*>(root.childAt(0));
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->tag(), "h1");
    EXPECT_EQ(first->parent(), &root);

    Document document;
    document.setRoot(std::make_unique<Element>(std::move(root)));
    EXPECT_NE(first->ownerDocument(), nullptr);
    EXPECT_NE(first->uid(), 0U);
    EXPECT_EQ(document.root().childCount(), 2U);
}

TEST(Core, FindAndTraverse) {
    auto root = div().id("root").child(div().className("card").id("card1").child(h1().text("A")))
                    .child(div().className("card").id("card2"));
    Document document;
    document.setRoot(std::make_unique<Element>(std::move(root)));

    EXPECT_NE(document.findById("card2"), nullptr);
    EXPECT_EQ(document.findById("missing"), nullptr);
    EXPECT_EQ(document.findByClass("card").size(), 2U);
    EXPECT_EQ(document.findByTag("h1").size(), 1U);

    int elements = 0;
    int textNodes = 0;
    document.traverse([&](Node& node) {
        if (node.isElement()) {
            ++elements;
        } else {
            ++textNodes;
        }
    });
    EXPECT_EQ(elements, 4);
    EXPECT_EQ(textNodes, 1);
}

TEST(Core, AppendDetachRemoveClear) {
    Document document;
    auto root = document.createElement("div");
    auto child = document.createElement("span");
    auto* childPointer = child.get();
    root->appendChild(std::move(child));
    EXPECT_EQ(root->childCount(), 1U);
    EXPECT_EQ(childPointer->parent(), root.get());

    auto detached = root->detachChild(*childPointer);
    ASSERT_NE(detached, nullptr);
    EXPECT_EQ(root->childCount(), 0U);
    EXPECT_EQ(detached->parent(), nullptr);
    EXPECT_EQ(detached->ownerDocument(), nullptr);

    root->appendChild(std::move(detached));
    EXPECT_TRUE(root->removeChild(*childPointer));
    EXPECT_EQ(root->childCount(), 0U);

    root->appendChild(std::make_unique<Element>("p"));
    root->appendChild(std::make_unique<Element>("p"));
    root->clearChildren();
    EXPECT_EQ(root->childCount(), 0U);
}

TEST(Core, TextContentAndClone) {
    auto root = div().child(span().text("Hello")).child(span().text(" World"));
    EXPECT_EQ(root.textContent(), "Hello World");

    auto clone = root.cloneElement();
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(clone->childCount(), 2U);
    EXPECT_EQ(clone->textContent(), "Hello World");
    EXPECT_EQ(clone->parent(), nullptr);
}

TEST(Core, MoveUpdatesChildParents) {
    auto page = div().child(h1().text("Title"));
    auto* heading = static_cast<Element*>(page.childAt(0));
    Element moved = std::move(page);
    EXPECT_EQ(heading->parent(), &moved);

    Element assigned("section");
    assigned = std::move(moved);
    EXPECT_EQ(heading->parent(), &assigned);
}

TEST(Core, DocumentDirtyFlags) {
    Document document;
    document.setRoot(std::make_unique<Element>("div"));
    document.clearDirty();
    EXPECT_FALSE(document.styleDirty());
    document.invalidatePaint();
    EXPECT_FALSE(document.styleDirty());
    EXPECT_FALSE(document.layoutDirty());
    EXPECT_TRUE(document.paintDirty());
    document.invalidateLayout();
    EXPECT_TRUE(document.layoutDirty());
    document.invalidateStyle();
    EXPECT_TRUE(document.styleDirty());
}

TEST(Core, CreateElementOwnedByDocument) {
    Document document;
    auto element = document.createElement("div");
    EXPECT_EQ(element->ownerDocument(), &document);
    EXPECT_NE(element->uid(), 0U);
    auto text = document.createTextNode("hello");
    EXPECT_EQ(text->ownerDocument(), &document);
}
