#pragma once

#include <memory>

#include "entroui/layout/box.hpp"

namespace EntroUI {

class Document;
class Node;
class LayoutBox;
class TextMeasurer;
class ImageSizeProvider;

class LayoutTree {
public:
    LayoutTree();
    ~LayoutTree();

    LayoutTree(const LayoutTree&) = delete;
    LayoutTree& operator=(const LayoutTree&) = delete;

    void layout(Document& document, float viewportWidth, float viewportHeight,
                TextMeasurer& textMeasurer, ImageSizeProvider* imageProvider = nullptr);

    [[nodiscard]] const LayoutBox* root() const;
    [[nodiscard]] LayoutBox* root();
    [[nodiscard]] const LayoutBox* find(const Node* node) const;
    [[nodiscard]] LayoutBox* find(const Node* node);
    [[nodiscard]] const LayoutBox* hitTest(float x, float y) const;

    [[nodiscard]] bool valid() const;

private:
    [[nodiscard]] const LayoutBox* hitTestRecursive(const LayoutBox* box, float x, float y,
                                                    const Rect* clip) const;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace EntroUI
