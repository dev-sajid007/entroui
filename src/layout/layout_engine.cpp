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

AlignSelf effectiveAlign(AlignSelf self, AlignItems items) {
    if (self != AlignSelf::Auto) {
        return self;
    }
    switch (items) {
    case AlignItems::Start:
        return AlignSelf::Start;
    case AlignItems::Center:
        return AlignSelf::Center;
    case AlignItems::End:
        return AlignSelf::End;
    case AlignItems::Stretch:
        return AlignSelf::Stretch;
    }
    return AlignSelf::Start;
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
        const float originX = box->border.left + box->padding.left;
        const float originY = box->border.top + box->padding.top;
        const FontSpec font = fontFor(style);
        const float spaceWidth = textWidth(" ", font);
        const float baseLineHeight = style.resolvedLineHeight();

        struct InlineEntry {
            enum class Kind { Word, Space, Element };

            Kind kind = Kind::Word;
            const TextNode* textNode = nullptr;
            std::string word;
            float width = 0.0F;
            std::unique_ptr<LayoutBox> elementBox;
        };

        std::vector<InlineEntry> line;
        float lineCursor = 0.0F;

        auto flushLine = [&]() {
            if (line.empty()) {
                lineCursor = 0.0F;
                return;
            }
            while (!line.empty() && line.back().kind == InlineEntry::Kind::Space) {
                line.pop_back();
            }
            if (line.empty()) {
                lineCursor = 0.0F;
                return;
            }

            float totalWidth = 0.0F;
            float lineHeight = baseLineHeight;
            for (const auto& entry : line) {
                if (entry.kind == InlineEntry::Kind::Space) {
                    totalWidth += spaceWidth;
                } else if (entry.kind == InlineEntry::Kind::Word) {
                    totalWidth += entry.width;
                } else if (entry.elementBox != nullptr) {
                    totalWidth += entry.elementBox->margin.horizontal() +
                                  entry.elementBox->borderBox.width;
                    lineHeight = std::max(lineHeight,
                                          entry.elementBox->borderBox.height +
                                              entry.elementBox->margin.vertical());
                }
            }

            float alignOffset = 0.0F;
            if (style.textAlign == TextAlign::Center) {
                alignOffset = std::max(0.0F, (contentWidth - totalWidth) * 0.5F);
            } else if (style.textAlign == TextAlign::Right) {
                alignOffset = std::max(0.0F, contentWidth - totalWidth);
            }

            float x = alignOffset;
            bool textActive = false;
            float textStart = 0.0F;
            float textWidthValue = 0.0F;
            std::string text;
            const TextNode* textOwner = nullptr;

            auto flushTextSegment = [&]() {
                if (!textActive || textOwner == nullptr) {
                    textActive = false;
                    text.clear();
                    textWidthValue = 0.0F;
                    return;
                }
                auto segment = std::make_unique<LayoutBox>();
                segment->node = textOwner;
                segment->textNode = textOwner;
                segment->isText = true;
                segment->font = font;
                segment->textColor = style.color;
                segment->textAlign = style.textAlign;
                segment->lineHeight = baseLineHeight;
                segment->lines.push_back(text);
                segment->borderBox = {originX + textStart,
                                      originY + cursorY + (lineHeight - baseLineHeight) * 0.5F,
                                      textWidthValue, baseLineHeight};
                segment->paddingBox = segment->borderBox;
                segment->contentBox = segment->borderBox;
                segment->parent = box;
                box->children.push_back(std::move(segment));
                textActive = false;
                text.clear();
                textWidthValue = 0.0F;
            };

            for (auto& entry : line) {
                if (entry.kind == InlineEntry::Kind::Space) {
                    if (textActive) {
                        text.push_back(' ');
                        textWidthValue += spaceWidth;
                    }
                    x += spaceWidth;
                    continue;
                }
                if (entry.kind == InlineEntry::Kind::Word) {
                    if (!textActive || textOwner != entry.textNode) {
                        flushTextSegment();
                        textActive = true;
                        textOwner = entry.textNode;
                        textStart = x;
                    }
                    text += entry.word;
                    textWidthValue += entry.width;
                    x += entry.width;
                    continue;
                }

                flushTextSegment();
                auto& childBox = entry.elementBox;
                if (childBox == nullptr) {
                    continue;
                }
                childBox->parent = box;
                const float childX = originX + x + childBox->margin.left;
                const float childY = originY + cursorY +
                                     (lineHeight - childBox->borderBox.height) * 0.5F;
                childBox->borderBox.translate(childX, childY);
                childBox->paddingBox.translate(childX, childY);
                childBox->contentBox.translate(childX, childY);
                translateChildren(childBox.get(), childX, childY);
                x += childBox->margin.horizontal() + childBox->borderBox.width;
                box->children.push_back(std::move(childBox));
            }
            flushTextSegment();

            cursorY += lineHeight;
            line.clear();
            lineCursor = 0.0F;
        };

        for (auto& child : element.children()) {
            if (child->isText()) {
                const auto& textNode = static_cast<const TextNode&>(*child);
                const std::string& textValue = textNode.text();
                if (textValue.empty()) {
                    continue;
                }
                std::size_t position = 0;
                while (position < textValue.size()) {
                    if (textValue[position] == ' ') {
                        if (!line.empty()) {
                            line.push_back(InlineEntry{InlineEntry::Kind::Space});
                            lineCursor += spaceWidth;
                        }
                        ++position;
                        continue;
                    }
                    std::size_t end = textValue.find(' ', position);
                    if (end == std::string::npos) {
                        end = textValue.size();
                    }
                    const std::string word = textValue.substr(position, end - position);
                    const float width = textWidth(word, font);
                    constexpr float kTextFitTolerance = 2.0F;
                    if (lineCursor + width > contentWidth + kTextFitTolerance &&
                        lineCursor > 0.0F) {
                        flushLine();
                    }
                    InlineEntry entry;
                    entry.kind = InlineEntry::Kind::Word;
                    entry.textNode = &textNode;
                    entry.word = word;
                    entry.width = width;
                    line.push_back(std::move(entry));
                    lineCursor += width;
                    position = end;
                }
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
            const bool inlineElement = childStyle.display == Display::Inline ||
                                       childStyle.display == Display::InlineBlock;
            if (inlineElement) {
                auto childBox = layoutElement(childElement, contentWidth, availableHeight, false,
                                              -1.0F, -1.0F);
                const float outerWidth =
                    childBox->margin.horizontal() + childBox->borderBox.width;
                if (lineCursor + outerWidth > contentWidth && lineCursor > 0.0F) {
                    flushLine();
                }
                InlineEntry entry;
                entry.kind = InlineEntry::Kind::Element;
                entry.elementBox = std::move(childBox);
                const float width = entry.elementBox->margin.horizontal() +
                                    entry.elementBox->borderBox.width;
                line.push_back(std::move(entry));
                lineCursor += width;
                continue;
            }

            flushLine();
            const bool fill = childStyle.width.isAuto();
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
        flushLine();

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
            const Element* element = nullptr;
            float grow = 0.0F;
            float shrink = 0.0F;
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
            if (!childStyle.flexBasis.isAuto() && row) {
                forcedWidth = childStyle.flexBasis.resolve(contentWidth);
            } else if (!childStyle.width.isAuto()) {
                forcedWidth = childStyle.width.resolve(contentWidth);
            } else if (!row) {
                const AlignSelf align =
                    effectiveAlign(childStyle.alignSelf, style.alignItems);
                if (align == AlignSelf::Stretch) {
                    forcedWidth =
                        std::max(0.0F, contentWidth - childMargin.horizontal());
                }
            }

            float forcedHeight = -1.0F;
            if (!childStyle.flexBasis.isAuto() && !row) {
                forcedHeight = childStyle.flexBasis.resolve(availableHeight);
            } else if (!childStyle.height.isAuto()) {
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
            layout.element = &childElement;
            layout.grow = childStyle.flexGrow;
            layout.shrink = childStyle.flexShrink;
            box->children.push_back(std::move(childBox));
            children.push_back(layout);
        }

        const float mainAvailable = row ? contentWidth : contentHeightForFlex(fixedHeight, box);
        const float mainGap = row ? style.resolvedColumnGap() : style.resolvedRowGap();
        const float crossGap = row ? style.resolvedRowGap() : style.resolvedColumnGap();

        auto relayoutMain = [&](ChildLayout& child, float newMain) {
            if (child.element == nullptr) {
                return;
            }
            auto& childElement = const_cast<Element&>(*child.element);
            const auto& childStyle = childElement.computedStyle();
            const EdgeInsets childMargin = childStyle.margin.resolve(contentWidth);
            float forcedWidth = -1.0F;
            float forcedHeight = -1.0F;
            if (row) {
                forcedWidth = std::max(0.0F, newMain);
                if (!childStyle.height.isAuto()) {
                    forcedHeight = childStyle.height.unit == LengthUnit::Percent
                                       ? childStyle.height.resolve(availableHeight, -1.0F)
                                       : childStyle.height.value;
                }
            } else {
                forcedHeight = std::max(0.0F, newMain);
                if (!childStyle.width.isAuto()) {
                    forcedWidth = childStyle.width.resolve(contentWidth);
                } else {
                    const AlignSelf align =
                        effectiveAlign(childStyle.alignSelf, style.alignItems);
                    if (align == AlignSelf::Stretch) {
                        forcedWidth =
                            std::max(0.0F, contentWidth - childMargin.horizontal());
                    }
                }
            }
            auto replacement = layoutElement(childElement, contentWidth, availableHeight, false,
                                             forcedWidth, forcedHeight);
            replacement->parent = box;
            const Node* node = child.box->node;
            child.mainSize =
                row ? replacement->borderBox.width : replacement->borderBox.height;
            child.crossSize =
                row ? replacement->borderBox.height : replacement->borderBox.width;
            for (auto& stored : box->children) {
                if (stored != nullptr && stored->node == node) {
                    stored = std::move(replacement);
                    child.box = stored.get();
                    break;
                }
            }
        };

        auto relayoutCrossRow = [&](ChildLayout& child, float targetCross) {
            if (child.element == nullptr || !child.autoCross) {
                return;
            }
            auto& childElement = const_cast<Element&>(*child.element);
            const float target = std::max(
                0.0F, targetCross - child.marginCrossStart - child.marginCrossEnd);
            auto replacement = layoutElement(childElement, contentWidth, availableHeight, false,
                                             child.mainSize, target);
            replacement->parent = box;
            const float replacementMain =
                row ? replacement->borderBox.width : replacement->borderBox.height;
            const Node* node = child.box->node;
            child.mainSize = replacementMain;
            child.crossSize =
                row ? replacement->borderBox.height : replacement->borderBox.width;
            for (auto& stored : box->children) {
                if (stored != nullptr && stored->node == node) {
                    stored = std::move(replacement);
                    child.box = stored.get();
                    break;
                }
            }
        };

        if (style.flexWrap == FlexWrap::Wrap && row && !children.empty()) {
            std::vector<std::vector<std::size_t>> lines;
            std::vector<std::size_t> currentLine;
            float currentLineWidth = 0.0F;
            for (std::size_t index = 0; index < children.size(); ++index) {
                const auto& child = children[index];
                const float outer =
                    child.marginMainStart + child.mainSize + child.marginMainEnd;
                const float increment = currentLine.empty()
                                            ? outer
                                            : mainGap + outer;
                if (mainAvailable > 0.0F && !currentLine.empty() &&
                    currentLineWidth + increment > mainAvailable + 0.01F) {
                    lines.push_back(std::move(currentLine));
                    currentLine.clear();
                    currentLineWidth = 0.0F;
                }
                currentLineWidth += currentLine.empty() ? outer : mainGap + outer;
                currentLine.push_back(index);
            }
            if (!currentLine.empty()) {
                lines.push_back(std::move(currentLine));
            }

            std::vector<float> lineCrossSizes(lines.size(), 0.0F);
            for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
                auto& indices = lines[lineIndex];
                float used = 0.0F;
                for (const std::size_t index : indices) {
                    used += children[index].marginMainStart + children[index].mainSize +
                            children[index].marginMainEnd;
                }
                if (indices.size() > 1) {
                    used += mainGap * static_cast<float>(indices.size() - 1);
                }

                if (mainAvailable > 0.0F) {
                    float totalGrow = 0.0F;
                    float totalShrinkWeight = 0.0F;
                    for (const std::size_t index : indices) {
                        totalGrow += children[index].grow;
                        totalShrinkWeight +=
                            children[index].shrink * children[index].mainSize;
                    }
                    const float freeMain = mainAvailable - used;
                    if (freeMain > 0.0F && totalGrow > 0.0F) {
                        for (const std::size_t index : indices) {
                            auto& child = children[index];
                            if (child.grow > 0.0F) {
                                relayoutMain(
                                    child,
                                    child.mainSize + freeMain * (child.grow / totalGrow));
                            }
                        }
                    } else if (freeMain < 0.0F && totalShrinkWeight > 0.0F) {
                        for (const std::size_t index : indices) {
                            auto& child = children[index];
                            if (child.shrink > 0.0F && child.mainSize > 0.0F) {
                                const float scaled = child.shrink * child.mainSize;
                                relayoutMain(
                                    child,
                                    child.mainSize +
                                        freeMain * (scaled / totalShrinkWeight));
                            }
                        }
                    }
                }

                for (const std::size_t index : indices) {
                    lineCrossSizes[lineIndex] =
                        std::max(lineCrossSizes[lineIndex],
                                 children[index].marginCrossStart +
                                     children[index].crossSize +
                                     children[index].marginCrossEnd);
                }
            }

            float totalCross = 0.0F;
            for (const float size : lineCrossSizes) {
                totalCross += size;
            }
            if (lines.size() > 1) {
                totalCross += crossGap * static_cast<float>(lines.size() - 1);
            }

            const float contentCrossAvailable =
                fixedHeight >= 0.0F
                    ? std::max(0.0F, fixedHeight - box->padding.vertical() -
                                         box->border.vertical())
                    : totalCross;
            float freeCross = std::max(0.0F, contentCrossAvailable - totalCross);
            float crossOffset = 0.0F;
            float extraCrossGap = 0.0F;
            if (fixedHeight >= 0.0F && style.alignContent != AlignContent::Stretch) {
                switch (style.alignContent) {
                case AlignContent::Start:
                    break;
                case AlignContent::Center:
                    crossOffset = freeCross * 0.5F;
                    break;
                case AlignContent::End:
                    crossOffset = freeCross;
                    break;
                case AlignContent::SpaceBetween:
                    if (lines.size() > 1) {
                        extraCrossGap =
                            freeCross / static_cast<float>(lines.size() - 1);
                    }
                    break;
                case AlignContent::SpaceAround:
                    if (!lines.empty()) {
                        const float slice =
                            freeCross / static_cast<float>(lines.size());
                        crossOffset = slice * 0.5F;
                        extraCrossGap = slice;
                    }
                    break;
                case AlignContent::Stretch:
                    break;
                }
                freeCross = 0.0F;
            }
            if (fixedHeight >= 0.0F && style.alignContent == AlignContent::Stretch &&
                !lines.empty()) {
                const float extra = freeCross / static_cast<float>(lines.size());
                for (float& size : lineCrossSizes) {
                    size += extra;
                }
                freeCross = 0.0F;
            }

            float lineTop = crossOffset;
            for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
                const auto& indices = lines[lineIndex];
                const float lineCross = lineCrossSizes[lineIndex];
                for (const std::size_t index : indices) {
                    auto& child = children[index];
                    AlignSelf align = AlignSelf::Auto;
                    if (child.element != nullptr) {
                        align = child.element->computedStyle().alignSelf;
                    }
                    align = effectiveAlign(align, style.alignItems);
                    if (align == AlignSelf::Stretch && child.autoCross) {
                        relayoutCrossRow(child, lineCross);
                    }
                }

                float used = 0.0F;
                for (const std::size_t index : indices) {
                    used += children[index].marginMainStart + children[index].mainSize +
                            children[index].marginMainEnd;
                }
                if (indices.size() > 1) {
                    used += mainGap * static_cast<float>(indices.size() - 1);
                }
                const float freeMain = std::max(0.0F, mainAvailable - used);
                float mainOffset = 0.0F;
                float justifyGap = mainGap;
                switch (style.justifyContent) {
                case JustifyContent::Start:
                    break;
                case JustifyContent::Center:
                    mainOffset = freeMain * 0.5F;
                    break;
                case JustifyContent::End:
                    mainOffset = freeMain;
                    break;
                case JustifyContent::SpaceBetween:
                    if (indices.size() > 1) {
                        justifyGap += freeMain / static_cast<float>(indices.size() - 1);
                    }
                    break;
                case JustifyContent::SpaceAround:
                    if (!indices.empty()) {
                        const float slice = freeMain / static_cast<float>(indices.size());
                        mainOffset = slice * 0.5F;
                        justifyGap += slice;
                    }
                    break;
                }

                float x = mainOffset;
                for (const std::size_t index : indices) {
                    auto& child = children[index];
                    AlignSelf align = AlignSelf::Auto;
                    if (child.element != nullptr) {
                        align = child.element->computedStyle().alignSelf;
                    }
                    align = effectiveAlign(align, style.alignItems);
                    const float outerCross =
                        child.marginCrossStart + child.crossSize + child.marginCrossEnd;
                    float crossPosition = child.marginCrossStart;
                    if (align == AlignSelf::Center) {
                        crossPosition += (lineCross - outerCross) * 0.5F;
                    } else if (align == AlignSelf::End) {
                        crossPosition += lineCross - outerCross;
                    }

                    const float originX = box->border.left + box->padding.left;
                    const float originY = box->border.top + box->padding.top;
                    const float targetX = originX + x + child.marginMainStart;
                    const float targetY = originY + lineTop + crossPosition;
                    child.box->borderBox.translate(targetX, targetY);
                    child.box->paddingBox.translate(targetX, targetY);
                    child.box->contentBox.translate(targetX, targetY);
                    translateChildren(child.box, targetX, targetY);
                    x += child.mainSize + child.marginMainEnd + justifyGap;
                }
                lineTop += lineCross + crossGap + extraCrossGap;
            }

            if (fixedHeight >= 0.0F) {
                return std::max(0.0F, fixedHeight - box->padding.vertical() -
                                          box->border.vertical());
            }
            return std::max(0.0F, lineTop - crossGap);
        }

        float used = 0.0F;
        for (const auto& child : children) {
            used += child.marginMainStart + child.mainSize + child.marginMainEnd;
        }
        if (!children.empty()) {
            used += mainGap * static_cast<float>(children.size() - 1);
        }

        if (mainAvailable > 0.0F) {
            float totalGrow = 0.0F;
            float totalShrinkWeight = 0.0F;
            for (const auto& child : children) {
                totalGrow += child.grow;
                totalShrinkWeight += child.shrink * child.mainSize;
            }
            const float freeMain = mainAvailable - used;
            if (freeMain > 0.0F && totalGrow > 0.0F) {
                for (auto& child : children) {
                    if (child.grow <= 0.0F) {
                        continue;
                    }
                    relayoutMain(child, child.mainSize + freeMain * (child.grow / totalGrow));
                }
            } else if (freeMain < 0.0F && totalShrinkWeight > 0.0F) {
                for (auto& child : children) {
                    if (child.shrink <= 0.0F || child.mainSize <= 0.0F) {
                        continue;
                    }
                    const float scaled = child.shrink * child.mainSize;
                    relayoutMain(child, child.mainSize + freeMain * (scaled / totalShrinkWeight));
                }
            }

            used = 0.0F;
            for (const auto& child : children) {
                used += child.marginMainStart + child.mainSize + child.marginMainEnd;
            }
            if (!children.empty()) {
                used += mainGap * static_cast<float>(children.size() - 1);
            }
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

        if (row && fixedHeight >= 0.0F) {
            for (auto& child : children) {
                AlignSelf align = AlignSelf::Auto;
                if (child.element != nullptr) {
                    align = child.element->computedStyle().alignSelf;
                }
                align = effectiveAlign(align, style.alignItems);
                if (align != AlignSelf::Stretch || !child.autoCross || child.box == nullptr ||
                    !child.box->isElementBox()) {
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
            AlignSelf align = AlignSelf::Auto;
            if (child.element != nullptr) {
                align = child.element->computedStyle().alignSelf;
            }
            align = effectiveAlign(align, style.alignItems);
            if (align == AlignSelf::Center) {
                crossPosition += crossFree * 0.5F;
            } else if (align == AlignSelf::End) {
                crossPosition += crossFree;
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
