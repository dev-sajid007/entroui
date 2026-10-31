#include "entroui/renderer/render_tree.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include "entroui/core/element.hpp"
#include "entroui/layout/layout_engine.hpp"
#include "entroui/layout/layout_node.hpp"
#include "entroui/style/style.hpp"

namespace EntroUI {

namespace {

Color placeholderColor() { return Color{156, 163, 175, 255}; }
Color scrollbarColor() { return Color{148, 163, 184, 180}; }

std::string inputDisplayText(const Element& element, bool& placeholder) {
    if (!element.value().empty()) {
        placeholder = false;
        return element.value();
    }
    const std::string& hint = element.attribute("placeholder");
    placeholder = !hint.empty();
    return hint;
}

void addElementCommands(RenderCommandList& commands, const LayoutBox& box,
                        const Element& element, const Element* focused) {
    const auto& style = element.computedStyle();
    const Rect& borderBox = box.borderBox;
    if (borderBox.width <= 0.0F || borderBox.height <= 0.0F) {
        return;
    }

    if (style.background.visible()) {
        commands.add(RectCommand{borderBox, style.background, style.borderRadius});
    }
    if (style.borderWidth > 0.0F && style.borderColor.visible()) {
        commands.add(
            BorderCommand{borderBox, style.borderColor, style.borderWidth, style.borderRadius});
    }

    if (element.tag() == "image") {
        const std::string& source = element.attribute("src");
        if (!source.empty()) {
            commands.add(ImageCommand{borderBox, source, style.borderRadius});
        }
    }

    if (element.isTextInput()) {
        bool placeholder = false;
        const std::string text = inputDisplayText(element, placeholder);
        const FontSpec font{style.fontFamily, style.fontSize, style.fontWeight};
        const float lineHeight = style.resolvedLineHeight();
        const float y = box.contentBox.y +
                        std::max(0.0F, (box.contentBox.height - lineHeight) * 0.5F);
        if (element.hasSelection()) {
            const int selectionStart =
                std::min(element.selectionStart(), element.selectionEnd());
            const int selectionEnd =
                std::max(element.selectionStart(), element.selectionEnd());
            const float charWidth = font.size * 0.5F;
            const float selectionX =
                box.contentBox.x + static_cast<float>(selectionStart) * charWidth;
            const float selectionWidth = std::min(
                static_cast<float>(selectionEnd - selectionStart) * charWidth,
                std::max(0.0F, box.contentBox.right() - selectionX));
            if (selectionWidth > 0.0F) {
                commands.add(RectCommand{
                    Rect{selectionX, y, selectionWidth, lineHeight},
                    Color{191, 219, 254, 255},
                    0.0F,
                });
            }
        }
        if (!text.empty()) {
            commands.add(TextCommand{Rect{box.contentBox.x, y, box.contentBox.width, lineHeight},
                                     text, font, placeholder ? placeholderColor() : style.color,
                                     TextAlign::Left, lineHeight});
        }
        if (focused == &element) {
            const float caretWidth = 1.5F;
            const float caretX =
                box.contentBox.x + static_cast<float>(element.caret()) * font.size * 0.5F;
            commands.add(RectCommand{
                Rect{caretX, y, caretWidth, lineHeight},
                style.color,
                0.0F,
            });
        }
    }

    if (element.isCheckbox() && element.checked()) {
        const Rect mark = box.contentBox.inset(std::min(box.contentBox.width,
                                                        box.contentBox.height) *
                                               0.28F);
        commands.add(RectCommand{mark, Color{255, 255, 255, 255}, 1.0F});
    }

    if (box.clip) {
        commands.add(PushClipCommand{box.clipRect});
    }
    for (const auto& child : box.children) {
        if (child->isText) {
            for (std::size_t lineIndex = 0; lineIndex < child->lines.size(); ++lineIndex) {
                const float y = child->contentBox.y +
                                static_cast<float>(lineIndex) * child->lineHeight;
                commands.add(TextCommand{
                    Rect{child->contentBox.x, y, child->contentBox.width, child->lineHeight},
                    child->lines[lineIndex], child->font, child->textColor, child->textAlign,
                    child->lineHeight});
            }
        } else if (child->element != nullptr) {
            addElementCommands(commands, *child, *child->element, focused);
        }
    }
    if (box.clip) {
        commands.add(PopClipCommand{});
    }

    if (box.scrollable && box.scrollContentHeight > box.contentBox.height + 0.5F) {
        const float trackWidth = 6.0F;
        const float trackX = borderBox.right() - trackWidth - 3.0F;
        const float trackY = box.contentBox.y;
        const float trackHeight = box.contentBox.height;
        if (trackHeight > 8.0F) {
            const float ratio =
                std::clamp(box.contentBox.height / box.scrollContentHeight, 0.05F, 1.0F);
            const float thumbHeight = std::max(18.0F, trackHeight * ratio);
            const float maxScroll =
                std::max(0.1F, box.scrollContentHeight - box.contentBox.height);
            const float thumbY =
                trackY + (box.scrollOffsetY / maxScroll) * (trackHeight - thumbHeight);
            commands.add(RectCommand{
                Rect{trackX, thumbY, trackWidth, thumbHeight}, scrollbarColor(), 3.0F});
        }
    }
}

} // namespace

RenderCommandList buildRenderCommands(const LayoutTree& layout, const Element* focusedElement) {
    RenderCommandList commands;
    const LayoutBox* root = layout.root();
    if (root == nullptr) {
        return commands;
    }
    if (root->isText) {
        for (std::size_t lineIndex = 0; lineIndex < root->lines.size(); ++lineIndex) {
            const float y = root->contentBox.y + static_cast<float>(lineIndex) * root->lineHeight;
            commands.add(TextCommand{
                Rect{root->contentBox.x, y, root->contentBox.width, root->lineHeight},
                root->lines[lineIndex], root->font, root->textColor, root->textAlign,
                root->lineHeight});
        }
        return commands;
    }
    if (root->element != nullptr) {
        addElementCommands(commands, *root, *root->element, focusedElement);
    }
    return commands;
}

} // namespace EntroUI
