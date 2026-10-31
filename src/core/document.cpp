#include "entroui/core/document.hpp"

#include <stdexcept>

#include "entroui/style/stylesheet.hpp"

namespace EntroUI {

Document::Document() = default;

Document::~Document() = default;

Element& Document::root() {
    if (!root_) {
        throw std::logic_error("Document has no root element");
    }
    return *root_;
}

const Element& Document::root() const {
    if (!root_) {
        throw std::logic_error("Document has no root element");
    }
    return *root_;
}

void Document::setRoot(std::unique_ptr<Element> root) {
    if (root_ != nullptr) {
        root_->detachFrom(this);
        root_->setParentInternal(nullptr);
    }
    root_ = std::move(root);
    if (root_ != nullptr) {
        root_->setParentInternal(nullptr);
        root_->attachTo(this);
    }
    invalidateStyle();
}

std::unique_ptr<Element> Document::takeRoot() {
    if (!root_) {
        return nullptr;
    }
    root_->detachFrom(this);
    root_->setParentInternal(nullptr);
    auto result = std::move(root_);
    invalidateStyle();
    return result;
}

std::unique_ptr<Element> Document::createElement(std::string tag) {
    auto element = std::make_unique<Element>(std::move(tag));
    element->attachTo(this);
    return element;
}

std::unique_ptr<TextNode> Document::createTextNode(std::string text) {
    auto node = std::make_unique<TextNode>(std::move(text));
    node->attachTo(this);
    return node;
}

Element* Document::findById(std::string_view id) {
    return root_ != nullptr ? root_->findById(id) : nullptr;
}

const Element* Document::findById(std::string_view id) const {
    return root_ != nullptr ? root_->findById(id) : nullptr;
}

std::vector<Element*> Document::findByClass(std::string_view className) {
    return root_ != nullptr ? root_->findByClass(className) : std::vector<Element*>{};
}

std::vector<Element*> Document::findByTag(std::string_view tag) {
    return root_ != nullptr ? root_->findByTag(tag) : std::vector<Element*>{};
}

void Document::traverse(const std::function<void(Node&)>& visitor) {
    if (root_ != nullptr) {
        root_->traverse(visitor);
    }
}

void Document::traverse(const std::function<void(const Node&)>& visitor) const {
    if (root_ != nullptr) {
        static_cast<const Element&>(*root_).traverse(visitor);
    }
}

void Document::traverseElements(const std::function<void(Element&)>& visitor) {
    if (root_ != nullptr) {
        root_->traverseElements(visitor);
    }
}

void Document::setStylesheet(std::shared_ptr<const StyleSheet> stylesheet) {
    stylesheet_ = std::move(stylesheet);
    invalidateStyle();
}

void Document::setViewport(float width, float height) {
    if (viewportWidth_ == width && viewportHeight_ == height) {
        return;
    }
    viewportWidth_ = width;
    viewportHeight_ = height;
    invalidateLayout();
}

void Document::invalidateStyle() {
    styleDirty_ = true;
    layoutDirty_ = true;
    paintDirty_ = true;
}

void Document::invalidateLayout() {
    layoutDirty_ = true;
    paintDirty_ = true;
}

void Document::invalidatePaint() { paintDirty_ = true; }

void Document::attachNode(Node& node) { node.attachTo(this); }

void Document::detachNode(Node& node) { node.detachFrom(this); }

Node* Document::findByUid(std::uint64_t uid) {
    Node* result = nullptr;
    if (root_ == nullptr) {
        return nullptr;
    }
    root_->traverse(std::function<void(Node&)>([&result, uid](Node& node) {
        if (result == nullptr && node.uid() == uid) {
            result = &node;
        }
    }));
    return result;
}

} // namespace EntroUI
