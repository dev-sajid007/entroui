#pragma once

#include <memory>
#include <string>
#include <vector>

#include "entroui/layout/box.hpp"
#include "entroui/style/style.hpp"

namespace EntroUI {

class Node;
class Element;
class TextNode;

struct LayoutBox {
    const Node* node = nullptr;
    const Element* element = nullptr;
    const TextNode* textNode = nullptr;
    bool isText = false;

    Rect borderBox;
    Rect paddingBox;
    Rect contentBox;
    EdgeInsets margin;
    EdgeInsets padding;
    EdgeInsets border;

    bool scrollable = false;
    bool clip = false;
    Rect clipRect;
    float scrollOffsetY = 0.0F;
    float scrollContentHeight = 0.0F;

    std::vector<std::string> lines;
    FontSpec font;
    Color textColor{0, 0, 0, 255};
    TextAlign textAlign = TextAlign::Left;
    float lineHeight = 0.0F;

    LayoutBox* parent = nullptr;
    std::vector<std::unique_ptr<LayoutBox>> children;

    [[nodiscard]] bool isElementBox() const noexcept { return element != nullptr; }
};

} // namespace EntroUI
