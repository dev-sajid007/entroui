#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "entroui/core/element.hpp"
#include "entroui/core/text_node.hpp"

namespace EntroUI {

class StyleSheet;

class Document {
public:
    Document();
    ~Document();

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    [[nodiscard]] bool hasRoot() const noexcept { return root_ != nullptr; }
    [[nodiscard]] Element& root();
    [[nodiscard]] const Element& root() const;

    void setRoot(std::unique_ptr<Element> root);
    [[nodiscard]] std::unique_ptr<Element> takeRoot();

    [[nodiscard]] std::unique_ptr<Element> createElement(std::string tag);
    [[nodiscard]] std::unique_ptr<TextNode> createTextNode(std::string text);

    [[nodiscard]] Element* findById(std::string_view id);
    [[nodiscard]] const Element* findById(std::string_view id) const;
    [[nodiscard]] std::vector<Element*> findByClass(std::string_view className);
    [[nodiscard]] std::vector<Element*> findByTag(std::string_view tag);

    void traverse(const std::function<void(Node&)>& visitor);
    void traverse(const std::function<void(const Node&)>& visitor) const;
    void traverseElements(const std::function<void(Element&)>& visitor);

    void setStylesheet(std::shared_ptr<const StyleSheet> stylesheet);
    [[nodiscard]] const StyleSheet* stylesheet() const noexcept { return stylesheet_.get(); }
    [[nodiscard]] std::shared_ptr<const StyleSheet> sharedStylesheet() const noexcept {
        return stylesheet_;
    }

    void setViewport(float width, float height);
    [[nodiscard]] float viewportWidth() const noexcept { return viewportWidth_; }
    [[nodiscard]] float viewportHeight() const noexcept { return viewportHeight_; }

    void markDirty() { invalidateStyle(); }
    void invalidateStyle();
    void invalidateLayout();
    void invalidatePaint();

    [[nodiscard]] bool styleDirty() const noexcept { return styleDirty_; }
    [[nodiscard]] bool layoutDirty() const noexcept { return layoutDirty_; }
    [[nodiscard]] bool paintDirty() const noexcept { return paintDirty_; }
    void clearStyleDirty() noexcept { styleDirty_ = false; }
    void clearLayoutDirty() noexcept { layoutDirty_ = false; }
    void clearPaintDirty() noexcept { paintDirty_ = false; }
    void clearDirty() noexcept {
        styleDirty_ = false;
        layoutDirty_ = false;
        paintDirty_ = false;
    }

    [[nodiscard]] std::uint64_t nextUid() noexcept { return nextUid_++; }
    void attachNode(Node& node);
    void detachNode(Node& node);
    [[nodiscard]] Node* findByUid(std::uint64_t uid);

private:
    std::unique_ptr<Element> root_;
    std::shared_ptr<const StyleSheet> stylesheet_;
    float viewportWidth_ = 0.0F;
    float viewportHeight_ = 0.0F;
    bool styleDirty_ = true;
    bool layoutDirty_ = true;
    bool paintDirty_ = true;
    std::uint64_t nextUid_ = 1;
};

} // namespace EntroUI
