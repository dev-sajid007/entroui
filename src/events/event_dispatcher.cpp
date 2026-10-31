#include "entroui/events/event_dispatcher.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "entroui/core/document.hpp"
#include "entroui/core/element.hpp"
#include "entroui/layout/layout_engine.hpp"
#include "entroui/layout/layout_node.hpp"

namespace EntroUI {

namespace {

std::size_t previousUtf8(const std::string& text, std::size_t position) {
    if (position == 0 || text.empty()) {
        return 0;
    }
    --position;
    while (position > 0 &&
           (static_cast<unsigned char>(text[position]) & 0xC0U) == 0x80U) {
        --position;
    }
    return position;
}

std::size_t nextUtf8(const std::string& text, std::size_t position) {
    if (position >= text.size()) {
        return text.size();
    }
    ++position;
    while (position < text.size() &&
           (static_cast<unsigned char>(text[position]) & 0xC0U) == 0x80U) {
        ++position;
    }
    return position;
}

bool pathContains(const std::vector<Element*>& path, const Element* element) {
    return std::find(path.begin(), path.end(), element) != path.end();
}

} // namespace

EventDispatcher::EventDispatcher(Document& document, LayoutTree& layout)
    : document_(document), layout_(layout) {}

Element* EventDispatcher::hitTest(float x, float y) const {
    const auto* box = layout_.hitTest(x, y);
    return box != nullptr ? const_cast<Element*>(box->element) : nullptr;
}

void EventDispatcher::setFocus(Element* element) {
    if (element == focused_) {
        return;
    }
    if (focused_ != nullptr) {
        focused_->removeClass("focus");
        focused_->setFocused(false);
        FocusEvent blur(EventType::Blur);
        blur.target = focused_;
        focused_->dispatchEvent(blur);
    }
    focused_ = element;
    if (focused_ != nullptr) {
        focused_->addClass("focus");
        focused_->setFocused(true);
        if (focused_->isTextInput()) {
            focused_->setCaret(static_cast<int>(focused_->value().size()));
        }
        FocusEvent focus(EventType::Focus);
        focus.target = focused_;
        focused_->dispatchEvent(focus);
        document_.invalidatePaint();
    }
}

void EventDispatcher::clearInteraction() {
    for (auto* element : hoverPath_) {
        if (element != nullptr) {
            element->removeClass("hover");
            element->setHovered(false);
        }
    }
    hoverPath_.clear();
    if (pressed_ != nullptr) {
        for (auto* element : pathToRoot(pressed_)) {
            element->removeClass("active");
        }
        pressed_->setPressed(false);
        pressed_ = nullptr;
    }
    setFocus(nullptr);
    hovered_ = nullptr;
    lastClickTarget_ = nullptr;
    clickCount_ = 0;
}

std::vector<Element*> EventDispatcher::pathToRoot(Element* element) const {
    std::vector<Element*> path;
    for (Node* node = element; node != nullptr; node = node->parent()) {
        if (node->isElement()) {
            path.push_back(static_cast<Element*>(node));
        }
    }
    return path;
}

std::vector<Element*> EventDispatcher::focusableElements() const {
    std::vector<Element*> elements;
    document_.traverseElements([&elements](Element& element) {
        if (element.hasComputedStyle() &&
            element.computedStyle().display == Display::None) {
            return;
        }
        if (element.focusable() || element.tag() == "input" || element.tag() == "button") {
            elements.push_back(&element);
        }
    });
    return elements;
}

bool EventDispatcher::isFocusable(const Element& element) const {
    if (element.hasComputedStyle() && element.computedStyle().display == Display::None) {
        return false;
    }
    return element.focusable() || element.tag() == "input" || element.tag() == "button";
}

void EventDispatcher::updateHover(Element* target) {
    std::vector<Element*> newPath = target != nullptr ? pathToRoot(target) : std::vector<Element*>{};

    for (auto* element : hoverPath_) {
        if (element != nullptr && !pathContains(newPath, element)) {
            element->removeClass("hover");
            element->setHovered(false);
            MouseEvent leave(EventType::MouseLeave, 0.0F, 0.0F);
            leave.target = element;
            element->dispatchEvent(leave);
        }
    }
    for (auto it = newPath.rbegin(); it != newPath.rend(); ++it) {
        Element* element = *it;
        if (element != nullptr && !pathContains(hoverPath_, element)) {
            element->addClass("hover");
            element->setHovered(true);
            MouseEvent enter(EventType::MouseEnter, 0.0F, 0.0F);
            enter.target = element;
            element->dispatchEvent(enter);
        }
    }

    hoverPath_ = std::move(newPath);
    hovered_ = target;
}

void EventDispatcher::dispatchMouse(EventType type, Element* target, float x, float y, int button,
                                    int clicks) {
    if (target == nullptr) {
        return;
    }
    MouseEvent event(type, x, y);
    event.button = button;
    event.clicks = clicks;
    target->dispatchEvent(event);
}

void EventDispatcher::handleMouseMove(float x, float y) {
    Element* target = hitTest(x, y);
    if (target != hovered_) {
        updateHover(target);
    }
    if (target != nullptr) {
        MouseEvent event(EventType::MouseMove, x, y);
        target->dispatchEvent(event);
    }
}

void EventDispatcher::handleMouseDown(float x, float y, int button) {
    Element* target = hitTest(x, y);
    if (target == nullptr || button != 1) {
        return;
    }
    pressed_ = target;
    target->setPressed(true);
    for (auto* element : pathToRoot(target)) {
        element->addClass("active");
    }
    dispatchMouse(EventType::MouseDown, target, x, y, button);

    Element* focusTarget = target;
    while (focusTarget != nullptr && !isFocusable(*focusTarget)) {
        focusTarget = focusTarget->parent() != nullptr &&
                              focusTarget->parent()->isElement()
                          ? static_cast<Element*>(focusTarget->parent())
                          : nullptr;
    }
    if (focusTarget != nullptr) {
        setFocus(focusTarget);
    }
}

void EventDispatcher::handleMouseUp(float x, float y, int button) {
    Element* target = hitTest(x, y);
    if (target != nullptr) {
        dispatchMouse(EventType::MouseUp, target, x, y, button);
    }
    if (pressed_ == nullptr) {
        return;
    }

    Element* pressed = pressed_;
    for (auto* element : pathToRoot(pressed)) {
        element->removeClass("active");
    }
    pressed->setPressed(false);
    pressed_ = nullptr;

    if (button == 1 && target == pressed) {
        const auto now = std::chrono::steady_clock::now();
        if (lastClickTarget_ == target &&
            now - lastClickTime_ < std::chrono::milliseconds(400)) {
            ++clickCount_;
        } else {
            clickCount_ = 1;
        }
        lastClickTarget_ = target;
        lastClickTime_ = now;
        triggerClick(target, x, y, clickCount_);
    }
}

void EventDispatcher::triggerClick(Element* target, float x, float y, int clicks) {
    if (target == nullptr) {
        return;
    }
    MouseEvent event(EventType::Click, x, y);
    event.clicks = clicks;
    target->dispatchEvent(event);
    if (event.defaultPrevented) {
        return;
    }
    applyDefaultAction(event);
}

void EventDispatcher::applyDefaultAction(Event& event) {
    Element* target = event.target;
    if (target == nullptr) {
        return;
    }
    if (target->isCheckbox()) {
        target->setChecked(!target->checked());
        target->toggleClass("checked", target->checked());
        Event change(EventType::Change);
        change.target = target;
        target->dispatchEvent(change);
    } else if (target->isTextInput()) {
        setFocus(target);
    }
}

void EventDispatcher::handleWheel(float x, float y, float deltaX, float deltaY) {
    Element* target = hitTest(x, y);
    if (target == nullptr) {
        return;
    }
    WheelEvent wheel(x, y, deltaX, deltaY);
    target->dispatchEvent(wheel);
    if (wheel.defaultPrevented) {
        return;
    }

    Element* scrollTarget = scrollableAncestor(target);
    if (scrollTarget == nullptr) {
        return;
    }
    const LayoutBox* box = layout_.find(scrollTarget);
    if (box == nullptr) {
        return;
    }
    const float maxScroll =
        std::max(0.0F, box->scrollContentHeight - box->contentBox.height);
    scrollTarget->setScroll(scrollTarget->scrollX(),
                            std::clamp(scrollTarget->scrollY() + deltaY, 0.0F, maxScroll));
}

Element* EventDispatcher::scrollableAncestor(Element* element) const {
    for (Element* current = element; current != nullptr;
         current = current->parent() != nullptr && current->parent()->isElement()
                       ? static_cast<Element*>(current->parent())
                       : nullptr) {
        if (!current->scrollable()) {
            continue;
        }
        const LayoutBox* box = layout_.find(current);
        if (box != nullptr &&
            box->scrollContentHeight > box->contentBox.height + 0.5F) {
            return current;
        }
    }
    return nullptr;
}

void EventDispatcher::handleKeyDown(int key, int scancode, int mods, bool repeat) {
    if (focused_ == nullptr) {
        return;
    }
    KeyEvent event(EventType::KeyDown, key, scancode, mods);
    event.repeat = repeat;
    event.target = focused_;
    focused_->dispatchEvent(event);
    if (event.defaultPrevented) {
        return;
    }

    if (key == Keys::Tab) {
        focusNext((mods & 1) != 0);
        return;
    }
    if (key == Keys::Escape) {
        setFocus(nullptr);
        return;
    }

    if (focused_ == nullptr) {
        return;
    }

    if (focused_->isTextInput()) {
        std::string text = focused_->value();
        std::size_t caret = static_cast<std::size_t>(std::clamp(
            focused_->caret(), 0, static_cast<int>(text.size())));
        bool changed = false;
        if (key == Keys::Backspace && caret > 0) {
            const std::size_t previous = previousUtf8(text, caret);
            text.erase(previous, caret - previous);
            caret = previous;
            changed = true;
        } else if (key == Keys::Delete && caret < text.size()) {
            const std::size_t next = nextUtf8(text, caret);
            text.erase(caret, next - caret);
            changed = true;
        } else if (key == Keys::Left) {
            caret = previousUtf8(text, caret);
        } else if (key == Keys::Right) {
            caret = nextUtf8(text, caret);
        } else if (key == Keys::Home) {
            caret = 0;
        } else if (key == Keys::End) {
            caret = text.size();
        }
        if (changed) {
            focused_->setValue(std::move(text));
            Event change(EventType::Change);
            change.target = focused_;
            focused_->dispatchEvent(change);
        }
        focused_->setCaret(static_cast<int>(caret));
        document_.invalidatePaint();
        return;
    }

    if (focused_->tag() == "button" && (key == Keys::Enter || key == Keys::Space)) {
        triggerClick(focused_, 0.0F, 0.0F, 1);
    }
}

void EventDispatcher::handleKeyUp(int key, int scancode, int mods) {
    if (focused_ != nullptr) {
        KeyEvent event(EventType::KeyUp, key, scancode, mods);
        event.target = focused_;
        focused_->dispatchEvent(event);
    }
}

void EventDispatcher::handleTextInput(std::string_view text) {
    if (focused_ == nullptr) {
        return;
    }
    TextInputEvent event{std::string(text)};
    event.target = focused_;
    focused_->dispatchEvent(event);
    if (event.defaultPrevented || !focused_->isTextInput()) {
        return;
    }

    std::string value = focused_->value();
    const int caret = std::clamp(focused_->caret(), 0, static_cast<int>(value.size()));
    value.insert(static_cast<std::size_t>(caret), std::string(text));
    focused_->setValue(std::move(value));
    focused_->setCaret(caret + static_cast<int>(text.size()));

    Event change(EventType::Change);
    change.target = focused_;
    focused_->dispatchEvent(change);
}

void EventDispatcher::focusNext(bool reverse) {
    const auto elements = focusableElements();
    if (elements.empty()) {
        return;
    }
    auto it = std::find(elements.begin(), elements.end(), focused_);
    std::ptrdiff_t index = 0;
    if (it != elements.end()) {
        index = std::distance(elements.begin(), it);
        index += reverse ? -1 : 1;
    } else if (reverse) {
        index = static_cast<std::ptrdiff_t>(elements.size()) - 1;
    }
    if (index < 0) {
        index = static_cast<std::ptrdiff_t>(elements.size()) - 1;
    }
    index %= static_cast<std::ptrdiff_t>(elements.size());
    setFocus(elements[static_cast<std::size_t>(index)]);
}

void EventDispatcher::dispatch(Event& event) {
    if (event.target != nullptr) {
        event.target->dispatchEvent(event);
    }
}

void EventDispatcher::updateHoverClasses(const std::vector<Element*>& path, bool hovered) {
    for (auto* element : path) {
        if (element != nullptr) {
            element->toggleClass("hover", hovered);
            element->setHovered(hovered);
        }
    }
}

} // namespace EntroUI
