#include "entroui/layout/layout_engine.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "entroui/core/document.hpp"
#include "entroui/core/element.hpp"
#include "entroui/core/text_node.hpp"
#include "entroui/layout/layout_node.hpp"
#include "entroui/layout/text_measurer.hpp"
#include "entroui/style/computed_style.hpp"

namespace EntroUI {

namespace {

constexpr float kEpsilon = 0.01F;

float clampLength(const StyleProperties& style, float value, float reference) {
    if (!style.minWidth.isAuto()) {
        value = std::max(value, style.minWidth.resolve(reference));
    }
    if (!style.maxWidth.isAuto()) {
        value = std::min(value, style.maxWidth.resolve(reference));
    }
    return value;
}

float clampHeight(const StyleProperties& style, float value, float reference) {
    if (!style.minHeight.isAuto()) {
        value = std::max(value, style.minHeight.resolve(reference));
    }
    if (!style.maxHeight.isAuto()) {
        value = std::min(value, style.maxHeight.resolve(reference));
    }
    return value;
}

bool isVisible(const Element& element) {
    return !element.hasComputedStyle() || element.computedStyle().display != Display::None;
}

class LayoutBuilder {
public:
    LayoutBuilder(TextMeasurer& textMeasurer, ImageSizeProvider* imageProvider)
        : textMeasurer_(textMeasurer), imageProvider_(imageProvider) {}

    std::unique_ptr<LayoutBox> buildRoot(Element& root, float width, float height) {
        const auto& style = root.computedStyle();
        const float forcedHeight = style.height.isAuto() ? height : -1.0F;
        auto box = layoutElement(root, width, height, true, -1.0F, forcedHeight);
        return box;
    }

    void collect(LayoutBox* box, std::unordered_map<const Node*, LayoutBox*>& map) {
        if (box == nullptr) {
            return;
        }
        if (box->node != nullptr) {
            map[box->node] = box;
        }
        for (auto& child : box->children) {
            collect(child.get(), map);
        }
    }

private:
    float textWidth(std::string_view text, const FontSpec& font) {
        return textMeasurer_.measure(text, font).width;
    }

    FontSpec fontFor(const StyleProperties& style) const {
        return FontSpec{style.fontFamily, style.fontSize, style.fontWeight};
    }

    std::vector<std::string> wrapText(std::string_view text, const FontSpec& font,
                                      float maxWidth) {
        std::vector<std::string> lines;
        std::string normalized(text);
        normalized.erase(std::remove(normalized.begin(), normalized.end(), '\r'), normalized.end());

        std::size_t paragraphStart = 0;
        while (paragraphStart <= normalized.size()) {
            std::size_t newline = normalized.find('\n', paragraphStart);
            const bool last = newline == std::string::npos;
            if (last) {
                newline = normalized.size();
            }
            const std::string paragraph = normalized.substr(paragraphStart, newline - paragraphStart);
            std::size_t position = 0;
            std::string line;
            bool hasContent = false;
            while (position <= paragraph.size()) {
                while (position < paragraph.size() && paragraph[position] == ' ') {
                    ++position;
                }
                std::size_t end = paragraph.find(' ', position);
                if (end == std::string::npos) {
                    end = paragraph.size();
                }
                const std::string word = paragraph.substr(position, end - position);
                if (!word.empty()) {
                    const std::string candidate = hasContent ? line + " " + word : word;
                    const bool fits = maxWidth <= 1.0F || textWidth(candidate, font) <= maxWidth;
                    if (!hasContent || fits) {
                        line = candidate;
                        hasContent = true;
                    } else {
                        lines.push_back(line);
                        line = word;
                    }
                }
                if (end >= paragraph.size()) {
                    break;
                }
                position = end + 1;
            }
            if (hasContent) {
                lines.push_back(line);
            } else if (!last || paragraph.empty()) {
                lines.push_back(std::string());
            }
            if (last) {
                break;
            }
            paragraphStart = newline + 1;
        }
        return lines;
    }

    std::unique_ptr<LayoutBox> layoutText(const TextNode& textNode, const StyleProperties& style,
                                          float maxWidth, bool wrap) {
        const FontSpec font = fontFor(style);
        const float lineHeight = style.resolvedLineHeight();
        auto box = std::make_unique<LayoutBox>();
        box->node = &textNode;
        box->textNode = &textNode;
        box->isText = true;
        box->font = font;
        box->textColor = style.color;
        box->textAlign = style.textAlign;
        box->lineHeight = lineHeight;

        const float effectiveMax = wrap ? std::max(1.0F, maxWidth) : 100000.0F;
        box->lines = wrapText(textNode.text(), font, effectiveMax);

        float width = maxWidth;
        if (!wrap) {
            width = 0.0F;
            for (const auto& line : box->lines) {
                width = std::max(width, textWidth(line, font));
            }
            width = std::min(width, std::max(0.0F, maxWidth));
        }
        width = std::max(0.0F, width);
        const float height = static_cast<float>(box->lines.size()) * lineHeight;
        box->borderBox = {0.0F, 0.0F, width, height};
        box->paddingBox = box->borderBox;
        box->contentBox = box->borderBox;
        return box;
    }

    float intrinsicWidth(const Element& element) {
        const auto& style = element.computedStyle();
        if (!style.width.isAuto()) {
            if (style.width.unit == LengthUnit::Px) {
                return clampLength(style, style.width.value, 0.0F);
            }
            return 0.0F;
        }
        if (style.display == Display::None) {
            return 0.0F;
        }

        float content = 0.0F;
        if (element.tag() == "image") {
            if (imageProvider_ != nullptr) {
                if (auto size = imageProvider_->imageSize(element.attribute("src"))) {
                    content = size->width;
                }
            }
        } else if (element.isTextInput()) {
            const FontSpec font = fontFor(style);
            const std::string sample = !element.value().empty() ? element.value()
                                                                : element.attribute("placeholder");
            content = textWidth(sample, font);
        } else if (style.display == Display::Flex && style.flexDirection == FlexDirection::Row) {
            bool first = true;
            for (const auto& child : element.children()) {
                if (child->isElement()) {
                    const auto& childElement = static_cast<const Element&>(*child);
                    if (!isVisible(childElement)) {
                        continue;
                    }
                    content += childElement.computedStyle().margin.resolve(0.0F).horizontal() +
                               intrinsicWidth(childElement);
                } else if (child->isText()) {
                    const auto& textNode = static_cast<const TextNode&>(*child);
                    content += textWidth(textNode.text(), fontFor(style));
                }
                if (!first) {
                    content += style.gap;
                }
                first = false;
            }
        } else {
            for (const auto& child : element.children()) {
                if (child->isElement()) {
                    const auto& childElement = static_cast<const Element&>(*child);
                    if (!isVisible(childElement)) {
                        continue;
                    }
                    content = std::max(content,
                                       childElement.computedStyle().margin.resolve(0.0F).horizontal() +
                                           intrinsicWidth(childElement));
                } else if (child->isText()) {
                    const auto& textNode = static_cast<const TextNode&>(*child);
                    content =
                        std::max(content, textWidth(textNode.text(), fontFor(style)));
                }
            }
        }

        const auto padding = style.padding.resolve(0.0F);
        const float border = style.borderWidth * 2.0F;
        content += padding.horizontal() + border;
        if (element.isTextInput()) {
            content = std::max(content, 60.0F);
        }
        content = clampLength(style, content, 0.0F);
        return std::max(0.0F, content);
    }

    std::unique_ptr<LayoutBox> layoutElement(Element& element, float availableWidth,
                                             float availableHeight, bool fillWidth,
                                             float forcedWidth, float forcedHeight) {
        const auto& style = element.computedStyle();
        auto box = std::make_unique<LayoutBox>();
        box->node = &element;
        box->element = &element;
        box->scrollable = element.scrollable();
        box->scrollOffsetY = element.scrollY();

        if (style.display == Display::None) {
            box->borderBox = {};
            box->paddingBox = {};
            box->contentBox = {};
            return box;
        }

        box->padding = style.padding.resolve(availableWidth);
        box->margin = style.margin.resolve(availableWidth);
        box->border = {style.borderWidth, style.borderWidth, style.borderWidth,
                       style.borderWidth};

        float width = 0.0F;
        if (forcedWidth >= 0.0F) {
            width = forcedWidth;
        } else if (!style.width.isAuto()) {
            width = style.width.resolve(availableWidth);
        } else if (fillWidth) {
            width = std::max(0.0F, availableWidth - box->margin.horizontal());
        } else {
            width = intrinsicWidth(element);
        }
        width = clampLength(style, width, availableWidth);
        width = std::max(width, box->padding.horizontal() + box->border.horizontal());

        const float contentWidth =
            std::max(0.0F, width - box->padding.horizontal() - box->border.horizontal());

        float fixedHeight = -1.0F;
        if (forcedHeight >= 0.0F) {
            fixedHeight = forcedHeight;
        } else if (!style.height.isAuto()) {
            fixedHeight = style.height.unit == LengthUnit::Percent
                              ? style.height.resolve(availableHeight, -1.0F)
                              : style.height.value;
            if (fixedHeight < 0.0F) {
                fixedHeight = -1.0F;
            }
        }
        if (fixedHeight >= 0.0F) {
            fixedHeight = clampHeight(style, fixedHeight, availableHeight);
            fixedHeight = std::max(fixedHeight,
                                   box->padding.vertical() + box->border.vertical());
        }

        float contentHeight = 0.0F;
        float naturalHeight = 0.0F;
        const float childAvailableHeight =
            fixedHeight >= 0.0F
                ? std::max(0.0F, fixedHeight - box->padding.vertical() - box->border.vertical())
                : availableHeight;

        if (style.display == Display::Flex) {
            contentHeight = layoutFlex(element, box.get(), style, contentWidth,
                                       childAvailableHeight, fixedHeight);
        } else {
            contentHeight =
                layoutBlock(element, box.get(), style, contentWidth, childAvailableHeight);
        }
        naturalHeight = contentHeight;

        if (element.tag() == "image" && fixedHeight < 0.0F && contentHeight <= kEpsilon &&
            imageProvider_ != nullptr) {
            if (auto size = imageProvider_->imageSize(element.attribute("src"));
                size && size->width > 0.0F) {
                contentHeight = contentWidth * (size->height / size->width);
            }
        }

        if (fixedHeight >= 0.0F) {
            contentHeight = std::max(
                0.0F, fixedHeight - box->padding.vertical() - box->border.vertical());
        }

        float height = fixedHeight >= 0.0F
                           ? fixedHeight
                           : contentHeight + box->padding.vertical() + box->border.vertical();
        if (fixedHeight < 0.0F) {
            height = clampHeight(style, height, availableHeight);
        }

        box->borderBox = {0.0F, 0.0F, width, height};
        box->paddingBox = {box->border.left, box->border.top,
                           std::max(0.0F, width - box->border.horizontal()),
                           std::max(0.0F, height - box->border.vertical())};
        box->contentBox = {box->border.left + box->padding.left,
                           box->border.top + box->padding.top, contentWidth, contentHeight};

        if (box->scrollable) {
            box->scrollContentHeight = naturalHeight;
            box->clip = naturalHeight > contentHeight + kEpsilon;
            box->clipRect = box->contentBox;
            if (box->clip) {
                const float maxScroll = std::max(0.0F, naturalHeight - contentHeight);
                box->scrollOffsetY = std::clamp(element.scrollY(), 0.0F, maxScroll);
                for (auto& child : box->children) {
                    child->borderBox.translate(0.0F, -box->scrollOffsetY);
                    child->paddingBox.translate(0.0F, -box->scrollOffsetY);
                    child->contentBox.translate(0.0F, -box->scrollOffsetY);
                    translateChildren(child.get(), 0.0F, -box->scrollOffsetY);
                }
            }
        }

        return box;
    }

    static void translateChildren(LayoutBox* box, float dx, float dy) {
        if (box == nullptr) {
            return;
        }
        for (auto& child : box->children) {
            child->borderBox.translate(dx, dy);
            child->paddingBox.translate(dx, dy);
            child->contentBox.translate(dx, dy);
            translateChildren(child.get(), dx, dy);
        }
    }

    float layoutBlock(Element& element, LayoutBox* box, const StyleProperties& style,
                      float contentWidth, float availableHeight) {
        float cursorY = 0.0F;
        float maxBottom = 0.0F;
        const float originX = box->border.left + box->padding.left;
        const float originY = box->border.top + box->padding.top;

        for (auto& child : element.children()) {
            if (child->isText()) {
                const auto& textNode = static_cast<const TextNode&>(*child);
                auto textBox = layoutText(textNode, style, contentWidth, true);
                textBox->parent = box;
                const float x = 0.0F;
                const float y = cursorY;
                textBox->borderBox.translate(originX + x, originY + y);
                textBox->paddingBox = textBox->borderBox;
                textBox->contentBox = textBox->borderBox;
                cursorY += textBox->borderBox.height;
                maxBottom = std::max(maxBottom, cursorY);
                box->children.push_back(std::move(textBox));
                continue;
            }
            if (!child->isElement()) {
                continue;
            }
            auto& childElement = static_cast<Element&>(*child);
            if (!isVisible(childElement)) {
                continue;
            }
            const auto& childStyle = childElement.computedStyle();
            const bool fill = childStyle.width.isAuto() &&
                              childStyle.display != Display::InlineBlock;
            auto childBox = layoutElement(childElement, contentWidth, availableHeight, fill, -1.0F,
                                          -1.0F);
            childBox->parent = box;
            const float x = childBox->margin.left;
            const float y = cursorY + childBox->margin.top;
            childBox->borderBox.translate(originX + x, originY + y);
            childBox->paddingBox.translate(originX + x, originY + y);
            childBox->contentBox.translate(originX + x, originY + y);
            translateChildren(childBox.get(), originX + x, originY + y);
            cursorY += childBox->margin.top + childBox->borderBox.height +
                       childBox->margin.bottom;
            box->children.push_back(std::move(childBox));
        }

        return std::max(0.0F, cursorY);
    }

    float layoutFlex(Element& element, LayoutBox* box, const StyleProperties& style,
                     float contentWidth, float availableHeight, float fixedHeight) {
        const bool row = style.flexDirection == FlexDirection::Row;
        struct ChildLayout {
            LayoutBox* box = nullptr;
            float mainSize = 0.0F;
            float crossSize = 0.0F;
            float marginMainStart = 0.0F;
            float marginMainEnd = 0.0F;
            float marginCrossStart = 0.0F;
            float marginCrossEnd = 0.0F;
            bool autoMain = false;
            bool autoCross = false;
        };

        std::vector<ChildLayout> children;
        children.reserve(element.children().size());

        for (auto& child : element.children()) {
            if (child->isText()) {
                const auto& textNode = static_cast<const TextNode&>(*child);
                const float limit = row ? contentWidth : contentWidth;
                auto textBox = layoutText(textNode, style, limit, !row);
                textBox->parent = box;
                ChildLayout layout;
                layout.box = textBox.get();
                layout.mainSize = row ? textBox->borderBox.width : textBox->borderBox.height;
                layout.crossSize = row ? textBox->borderBox.height : textBox->borderBox.width;
                layout.autoMain = true;
                layout.autoCross = true;
                box->children.push_back(std::move(textBox));
                children.push_back(layout);
                continue;
            }
            if (!child->isElement()) {
                continue;
            }
            auto& childElement = static_cast<Element&>(*child);
            if (!isVisible(childElement)) {
                continue;
            }
            const auto& childStyle = childElement.computedStyle();
            const EdgeInsets childMargin = childStyle.margin.resolve(contentWidth);

            float forcedWidth = -1.0F;
            if (!childStyle.width.isAuto()) {
                forcedWidth = childStyle.width.resolve(contentWidth);
            } else if (!row && style.alignItems == AlignItems::Stretch) {
                forcedWidth = std::max(0.0F, contentWidth - childMargin.horizontal());
            }

            float forcedHeight = -1.0F;
            if (!childStyle.height.isAuto()) {
                forcedHeight = childStyle.height.unit == LengthUnit::Percent
                                   ? childStyle.height.resolve(availableHeight, -1.0F)
                                   : childStyle.height.value;
                if (forcedHeight < 0.0F) {
                    forcedHeight = -1.0F;
                }
            }

            auto childBox = layoutElement(childElement, contentWidth, availableHeight, false,
                                          forcedWidth, forcedHeight);
            childBox->parent = box;

            ChildLayout layout;
            layout.box = childBox.get();
            layout.mainSize = row ? childBox->borderBox.width : childBox->borderBox.height;
            layout.crossSize = row ? childBox->borderBox.height : childBox->borderBox.width;
            layout.marginMainStart = row ? childMargin.left : childMargin.top;
            layout.marginMainEnd = row ? childMargin.right : childMargin.bottom;
            layout.marginCrossStart = row ? childMargin.top : childMargin.left;
            layout.marginCrossEnd = row ? childMargin.bottom : childMargin.right;
            layout.autoMain = childStyle.width.isAuto() && row
                                  ? true
                                  : (childStyle.height.isAuto() && !row);
            layout.autoCross = row ? childStyle.height.isAuto() : childStyle.width.isAuto();
            box->children.push_back(std::move(childBox));
            children.push_back(layout);
        }

        const float mainAvailable = row ? contentWidth : contentHeightForFlex(fixedHeight, box);
        const float mainGap = style.gap;

        float used = 0.0F;
        for (const auto& child : children) {
            used += child.marginMainStart + child.mainSize + child.marginMainEnd;
        }
        if (!children.empty()) {
            used += mainGap * static_cast<float>(children.size() - 1);
        }

        const float free = std::max(0.0F, mainAvailable - used);
        float startOffset = 0.0F;
        float betweenGap = mainGap;
        switch (style.justifyContent) {
        case JustifyContent::Start:
            break;
        case JustifyContent::Center:
            startOffset = free * 0.5F;
            break;
        case JustifyContent::End:
            startOffset = free;
            break;
        case JustifyContent::SpaceBetween:
            if (children.size() > 1) {
                betweenGap += free / static_cast<float>(children.size() - 1);
            }
            break;
        case JustifyContent::SpaceAround:
            if (!children.empty()) {
                const float slice = free / static_cast<float>(children.size());
                startOffset = slice * 0.5F;
                betweenGap += slice;
            }
            break;
        }

        float crossAvailable = 0.0F;
        if (row) {
            crossAvailable = fixedHeight >= 0.0F
                                 ? std::max(0.0F, fixedHeight - box->padding.vertical() -
                                                      box->border.vertical())
                                 : 0.0F;
        } else {
            crossAvailable = contentWidth;
        }

        float crossExtent = 0.0F;
        for (const auto& child : children) {
            crossExtent = std::max(crossExtent, child.marginCrossStart + child.crossSize +
                                                    child.marginCrossEnd);
        }
        if (row && fixedHeight < 0.0F) {
            crossAvailable = crossExtent;
        }
        if (!row) {
            crossAvailable = contentWidth;
        }

        if (row && style.alignItems == AlignItems::Stretch && fixedHeight >= 0.0F) {
            for (auto& child : children) {
                if (!child.autoCross || child.box == nullptr || !child.box->isElementBox()) {
                    continue;
                }
                auto& childElement = const_cast<Element&>(*child.box->element);
                const float target = std::max(0.0F, crossAvailable - child.marginCrossStart -
                                                       child.marginCrossEnd);
                const auto& childStyle = childElement.computedStyle();
                float forcedWidth = -1.0F;
                if (!childStyle.width.isAuto()) {
                    forcedWidth = childStyle.width.resolve(contentWidth);
                }
                auto relayout = layoutElement(childElement, contentWidth, availableHeight, false,
                                              forcedWidth, target);
                relayout->parent = box;
                const Node* node = child.box->node;
                child.crossSize = relayout->borderBox.height;
                for (auto& stored : box->children) {
                    if (stored != nullptr && stored->node == node) {
                        stored = std::move(relayout);
                        child.box = stored.get();
                        break;
                    }
                }
            }
        }

        float cursor = startOffset;
        for (auto& child : children) {
            float mainPosition = child.marginMainStart + cursor;
            float crossPosition = child.marginCrossStart;
            float crossFree = crossAvailable - child.marginCrossStart - child.crossSize -
                              child.marginCrossEnd;
            if (style.alignItems == AlignItems::Center) {
                crossPosition += crossFree * 0.5F;
            } else if (style.alignItems == AlignItems::End) {
                crossPosition += crossFree;
            } else if (style.alignItems == AlignItems::Stretch) {
                crossPosition += 0.0F;
            }

            const float originX = box->border.left + box->padding.left;
            const float originY = box->border.top + box->padding.top;
            const float x = row ? originX + mainPosition : originX + crossPosition;
            const float y = row ? originY + crossPosition : originY + mainPosition;
            child.box->borderBox.translate(x, y);
            child.box->paddingBox.translate(x, y);
            child.box->contentBox.translate(x, y);
            translateChildren(child.box, x, y);
            cursor += child.mainSize + child.marginMainEnd + betweenGap;
        }

        if (row) {
            if (fixedHeight >= 0.0F) {
                return std::max(0.0F, fixedHeight - box->padding.vertical() -
                                          box->border.vertical());
            }
            return crossExtent;
        }

        if (fixedHeight >= 0.0F) {
            return std::max(0.0F, fixedHeight - box->padding.vertical() - box->border.vertical());
        }
        return cursor;
    }

    float contentHeightForFlex(float fixedHeight, const LayoutBox* box) const {
        if (fixedHeight >= 0.0F) {
            return std::max(0.0F, fixedHeight - box->padding.vertical() - box->border.vertical());
        }
        return 0.0F;
    }

    TextMeasurer& textMeasurer_;
    ImageSizeProvider* imageProvider_ = nullptr;
};

} // namespace

struct LayoutTree::Impl {
    std::unique_ptr<LayoutBox> root;
    std::unordered_map<const Node*, LayoutBox*> nodes;
    float width = 0.0F;
    float height = 0.0F;
};

LayoutTree::LayoutTree() : impl_(std::make_unique<Impl>()) {}
LayoutTree::~LayoutTree() = default;

void LayoutTree::layout(Document& document, float viewportWidth, float viewportHeight,
                        TextMeasurer& textMeasurer, ImageSizeProvider* imageProvider) {
    impl_->root.reset();
    impl_->nodes.clear();
    impl_->width = viewportWidth;
    impl_->height = viewportHeight;

    if (!document.hasRoot()) {
        return;
    }

    if (!document.root().hasComputedStyle()) {
        computeDocumentStyles(document, document.stylesheet());
    }

    LayoutBuilder builder(textMeasurer, imageProvider);
    impl_->root = builder.buildRoot(document.root(), viewportWidth, viewportHeight);
    if (impl_->root != nullptr) {
        builder.collect(impl_->root.get(), impl_->nodes);
    }
}

const LayoutBox* LayoutTree::root() const { return impl_->root.get(); }
LayoutBox* LayoutTree::root() { return impl_->root.get(); }

const LayoutBox* LayoutTree::find(const Node* node) const {
    const auto it = impl_->nodes.find(node);
    return it == impl_->nodes.end() ? nullptr : it->second;
}

LayoutBox* LayoutTree::find(const Node* node) {
    const auto it = impl_->nodes.find(node);
    return it == impl_->nodes.end() ? nullptr : it->second;
}

const LayoutBox* LayoutTree::hitTest(float x, float y) const {
    if (impl_->root == nullptr) {
        return nullptr;
    }
    return hitTestRecursive(impl_->root.get(), x, y, nullptr);
}

const LayoutBox* LayoutTree::hitTestRecursive(const LayoutBox* box, float x, float y,
                                              const Rect* clip) const {
    if (box == nullptr) {
        return nullptr;
    }
    if (clip != nullptr && !clip->contains(x, y)) {
        return nullptr;
    }

    Rect childClip;
    const Rect* clipForChildren = clip;
    if (box->clip) {
        childClip = box->clipRect;
        if (clip != nullptr) {
            childClip = childClip.intersection(*clip);
        }
        clipForChildren = &childClip;
    }

    for (auto it = box->children.rbegin(); it != box->children.rend(); ++it) {
        if (const auto* hit = hitTestRecursive(it->get(), x, y, clipForChildren)) {
            return hit;
        }
    }

    if (box->element != nullptr && box->borderBox.contains(x, y)) {
        return box;
    }
    return nullptr;
}

bool LayoutTree::valid() const { return impl_->root != nullptr; }

} // namespace EntroUI
