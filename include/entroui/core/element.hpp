#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "entroui/core/node.hpp"
#include "entroui/events/event.hpp"
#include "entroui/events/event_target.hpp"
#include "entroui/state/state.hpp"
#include "entroui/style/style.hpp"

namespace EntroUI {

class Document;
class TextNode;

class Element : public Node, public EventTarget {
public:
    explicit Element(std::string tag);

    Element(Element&& other) noexcept;
    Element& operator=(Element&& other) noexcept;
    Element(const Element&) = delete;
    Element& operator=(const Element&) = delete;
    ~Element() override;

    // --- identity ---------------------------------------------------------
    [[nodiscard]] const std::string& tag() const noexcept { return tag_; }
    [[nodiscard]] const std::string& id() const noexcept { return id_; }
    Element&& id(std::string value);

    Element&& className(std::string values);
    Element&& addClass(std::string value);
    Element&& removeClass(std::string_view value);
    Element&& toggleClass(std::string_view value, bool enabled);
    [[nodiscard]] bool hasClass(std::string_view value) const;
    [[nodiscard]] const std::vector<std::string>& classes() const noexcept { return classes_; }

    Element&& attr(std::string name, std::string value);
    [[nodiscard]] bool hasAttribute(std::string_view name) const;
    [[nodiscard]] const std::string& attribute(std::string_view name) const;
    [[nodiscard]] const std::map<std::string, std::string>& attributes() const noexcept {
        return attributes_;
    }
    void removeAttribute(std::string_view name);

    // --- content ----------------------------------------------------------
    Element&& text(std::string value);
    [[nodiscard]] std::string textContent() const;

    Element&& style(std::string property, std::string value);
    Element&& style(std::string declarations);
    [[nodiscard]] const std::map<std::string, std::string>& inlineStyles() const noexcept {
        return inlineStyles_;
    }

    // --- tree -------------------------------------------------------------
    Element&& child(Element&& childElement);
    Element&& child(TextNode&& textNode);
    Element&& child(std::unique_ptr<Node> childNode);
    Element& appendChild(std::unique_ptr<Node> childNode);
    [[nodiscard]] bool removeChild(const Node& childNode);
    [[nodiscard]] std::unique_ptr<Node> detachChild(const Node& childNode);
    void clearChildren();

    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& children() const noexcept {
        return children_;
    }
    [[nodiscard]] std::size_t childCount() const noexcept { return children_.size(); }
    [[nodiscard]] Node* childAt(std::size_t index) const;

    [[nodiscard]] Element* findById(std::string_view value);
    [[nodiscard]] const Element* findById(std::string_view value) const;
    [[nodiscard]] std::vector<Element*> findByClass(std::string_view value);
    [[nodiscard]] std::vector<Element*> findByTag(std::string_view tagName);

    void traverse(const std::function<void(Node&)>& visitor);
    void traverse(const std::function<void(const Node&)>& visitor) const;
    void traverseElements(const std::function<void(Element&)>& visitor);

    // --- events -----------------------------------------------------------
    Element&& on(EventType type, EventCallback callback);
    Element&& onClick(EventCallback callback) { return on(EventType::Click, std::move(callback)); }
    Element&& onMouseDown(EventCallback callback) {
        return on(EventType::MouseDown, std::move(callback));
    }
    Element&& onMouseUp(EventCallback callback) {
        return on(EventType::MouseUp, std::move(callback));
    }
    Element&& onMouseMove(EventCallback callback) {
        return on(EventType::MouseMove, std::move(callback));
    }
    Element&& onMouseEnter(EventCallback callback) {
        return on(EventType::MouseEnter, std::move(callback));
    }
    Element&& onMouseLeave(EventCallback callback) {
        return on(EventType::MouseLeave, std::move(callback));
    }
    Element&& onKeyDown(EventCallback callback) {
        return on(EventType::KeyDown, std::move(callback));
    }
    Element&& onKeyUp(EventCallback callback) {
        return on(EventType::KeyUp, std::move(callback));
    }
    Element&& onTextInput(EventCallback callback) {
        return on(EventType::TextInput, std::move(callback));
    }
    Element&& onFocus(EventCallback callback) { return on(EventType::Focus, std::move(callback)); }
    Element&& onBlur(EventCallback callback) { return on(EventType::Blur, std::move(callback)); }
    Element&& onChange(EventCallback callback) {
        return on(EventType::Change, std::move(callback));
    }
    Element&& onMouseWheel(EventCallback callback) {
        return on(EventType::MouseWheel, std::move(callback));
    }

    [[nodiscard]] const std::vector<EventCallback>* listeners(EventType type) const;
    void dispatchEvent(Event& event) override;

    // --- reactive state ---------------------------------------------------
    template <typename T>
    Element&& bindText(State<T> source);

    // --- control state ----------------------------------------------------
    [[nodiscard]] const std::string& value() const noexcept { return value_; }
    void setValue(std::string value);
    [[nodiscard]] int caret() const noexcept { return caret_; }
    void setCaret(int caret) noexcept { caret_ = caret; }
    [[nodiscard]] bool checked() const noexcept { return checked_; }
    void setChecked(bool checked);
    [[nodiscard]] bool focusable() const noexcept { return focusable_; }
    void setFocusable(bool focusable) noexcept { focusable_ = focusable; }
    [[nodiscard]] bool focused() const noexcept { return focused_; }
    void setFocused(bool focused) noexcept { focused_ = focused; }
    [[nodiscard]] bool pressed() const noexcept { return pressed_; }
    void setPressed(bool pressed) noexcept { pressed_ = pressed; }
    [[nodiscard]] bool hovered() const noexcept { return hovered_; }
    void setHovered(bool hovered) noexcept { hovered_ = hovered; }

    [[nodiscard]] bool scrollable() const noexcept { return scrollable_; }
    void setScrollable(bool scrollable) noexcept { scrollable_ = scrollable; }
    [[nodiscard]] float scrollX() const noexcept { return scrollX_; }
    [[nodiscard]] float scrollY() const noexcept { return scrollY_; }
    void setScroll(float x, float y);
    [[nodiscard]] bool isInput() const noexcept { return tag_ == "input"; }
    [[nodiscard]] bool isCheckbox() const noexcept;
    [[nodiscard]] bool isTextInput() const noexcept;

    // --- styling cache ----------------------------------------------------
    [[nodiscard]] bool hasComputedStyle() const noexcept { return hasComputedStyle_; }
    [[nodiscard]] const StyleProperties& computedStyle() const noexcept { return computedStyle_; }
    [[nodiscard]] StyleProperties& mutableComputedStyle() noexcept { return computedStyle_; }
    void setHasComputedStyle(bool value) noexcept { hasComputedStyle_ = value; }

    // --- invalidation -----------------------------------------------------
    void markDirty();
    void markStyleDirty();
    void markLayoutDirty();
    void markPaintDirty();

    [[nodiscard]] std::unique_ptr<Node> clone() const override;
    [[nodiscard]] std::unique_ptr<Element> cloneElement() const;

    void setDocumentInternal(Document* document) noexcept;
    void setParentInternal(Node* parent) noexcept;
    void attachTo(Document* document) override;
    void detachFrom(Document* document) override;

private:
    void assignOwnership();
    void reindex();
    [[nodiscard]] bool hasInlineStyle(std::string_view property) const;

    std::string tag_;
    std::string id_;
    std::vector<std::string> classes_;
    std::map<std::string, std::string> attributes_;
    std::map<std::string, std::string> inlineStyles_;
    std::vector<std::unique_ptr<Node>> children_;
    std::map<EventType, std::vector<EventCallback>> listeners_;
    std::vector<std::shared_ptr<Binding>> bindings_;
    StyleProperties computedStyle_;
    bool hasComputedStyle_ = false;

    std::string value_;
    int caret_ = 0;
    bool checked_ = false;
    bool focusable_ = false;
    bool focused_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
    bool scrollable_ = false;
    float scrollX_ = 0.0F;
    float scrollY_ = 0.0F;
};

template <typename T>
class TextBinding : public Binding {
public:
    TextBinding(Element* target, State<T> source)
        : target_(target), source_(std::move(source)) {
        subscription_ = source_.subscribe([this](const T&) { refresh(); });
    }

    void refresh() override {
        if (target_ != nullptr) {
            target_->text(detail::toDisplayString(source_.get()));
        }
    }

    void rebind(Element* target) override { target_ = target; }

private:
    Element* target_;
    State<T> source_;
    Subscription subscription_;
};

template <typename T>
Element&& Element::bindText(State<T> source) {
    auto binding = std::make_shared<TextBinding<T>>(this, std::move(source));
    bindings_.push_back(binding);
    binding->refresh();
    return std::move(*this);
}

} // namespace EntroUI

namespace EntroUI {

[[nodiscard]] Element div();
[[nodiscard]] Element h1();
[[nodiscard]] Element h2();
[[nodiscard]] Element p();
[[nodiscard]] Element span();
[[nodiscard]] Element button(std::string text = {});
[[nodiscard]] Element input(std::string placeholder = {});
[[nodiscard]] Element label(std::string text = {});
[[nodiscard]] Element image(std::string source = {});
[[nodiscard]] Element list();
[[nodiscard]] TextNode textNode(std::string text);

} // namespace EntroUI
