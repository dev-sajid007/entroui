#include "entroui/events/event_dispatcher.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "entroui/core/document.hpp"
#include "entroui/core/element.hpp"
#include "entroui/layout/layout_engine.hpp"
#include "entroui/layout/layout_node.hpp"
#include "entroui/layout/text_measurer.hpp"

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

EventDispatcher::EventDispatcher(Document& document, LayoutTree& layout,
                                 TextMeasurer* textMeasurer)
    : document_(document), layout_(layout), textMeasurer_(textMeasurer) {}

void EventDispatcher::setClipboard(ClipboardGet getter, ClipboardSet setter) {
    clipboardGet_ = std::move(getter);
    clipboardSet_ = std::move(setter);
}

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
            selectionAnchor_ = focused_->caret();
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

int EventDispatcher::caretAt(Element& element, float x) const {
    const std::string& value = element.value();
    if (value.empty()) {
        return 0;
    }
    const LayoutBox* box = layout_.find(&element);
    float originX = 0.0F;
    if (box != nullptr) {
        originX = box->contentBox.x;
    }
    const auto& style = element.computedStyle();
    const FontSpec font{style.fontFamily, style.fontSize, style.fontWeight};
    const float target = x - originX;
    const float fallbackCharWidth = style.fontSize * 0.5F;

    int bestIndex = 0;
    float bestDistance = std::numeric_limits<float>::max();
    std::size_t position = 0;
    while (true) {
        const std::string prefix = value.substr(0, position);
        const float width =
            textMeasurer_ != nullptr
                ? textMeasurer_->measure(prefix, font).width
                : static_cast<float>(prefix.size()) * fallbackCharWidth;
        const float distance = std::fabs(width - target);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(position);
        }
        if (position >= value.size()) {
            break;
        }
        position = nextUtf8(value, position);
    }
    return bestIndex;
}

void EventDispatcher::eraseSelection(Element& element) {
    if (!element.hasSelection()) {
        return;
    }
    const int start = std::min(element.selectionStart(), element.selectionEnd());
    const int end = std::max(element.selectionStart(), element.selectionEnd());
    std::string value = element.value();
    value.erase(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start));
    element.setValue(std::move(value));
    element.setCaret(start);
    element.clearSelection();
}

void EventDispatcher::dispatchChange(Element& element) {
    Event change(EventType::Change);
    change.target = &element;
    element.dispatchEvent(change);
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
    if (pressed_ != nullptr && pressed_->isTextInput() && pressed_->focused()) {
        const int caret = caretAt(*pressed_, x);
        if (caret != selectionAnchor_) {
            pressed_->setSelection(selectionAnchor_, caret);
        } else {
            pressed_->clearSelection();
        }
        pressed_->setCaret(caret);
        document_.invalidatePaint();
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
        if (focusTarget->isTextInput()) {
            const int caret = caretAt(*focusTarget, x);
            focusTarget->clearSelection();
            focusTarget->setCaret(caret);
            selectionAnchor_ = caret;
            document_.invalidatePaint();
        }
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
        focusNext((mods & Mods::Shift) != 0);
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
        const bool shift = (mods & Mods::Shift) != 0;
        const bool ctrl = (mods & Mods::Ctrl) != 0;

        if (ctrl) {
            if (key == 'a' || key == 'A') {
                focused_->setSelection(0, static_cast<int>(focused_->value().size()));
                focused_->setCaret(static_cast<int>(focused_->value().size()));
                selectionAnchor_ = 0;
                document_.invalidatePaint();
                return;
            }
            if (key == 'c' || key == 'C') {
                if (clipboardSet_ && focused_->hasSelection()) {
                    clipboardSet_(focused_->selectedText());
                }
                return;
            }
            if (key == 'x' || key == 'X') {
                if (focused_->hasSelection()) {
                    if (clipboardSet_) {
                        clipboardSet_(focused_->selectedText());
                    }
                    eraseSelection(*focused_);
                    dispatchChange(*focused_);
                    document_.invalidatePaint();
                }
                return;
            }
            if (key == 'v' || key == 'V') {
                if (clipboardGet_) {
                    const std::string pasted = clipboardGet_();
                    if (!pasted.empty()) {
                        eraseSelection(*focused_);
                        std::string value = focused_->value();
                        const int caret =
                            std::clamp(focused_->caret(), 0, static_cast<int>(value.size()));
                        value.insert(static_cast<std::size_t>(caret), pasted);
                        focused_->setValue(std::move(value));
                        focused_->setCaret(caret + static_cast<int>(pasted.size()));
                        focused_->clearSelection();
                        dispatchChange(*focused_);
                        document_.invalidatePaint();
                    }
                }
                return;
            }
        }

        std::string text = focused_->value();
        std::size_t caret = static_cast<std::size_t>(std::clamp(
            focused_->caret(), 0, static_cast<int>(text.size())));
        bool changed = false;

        if ((key == Keys::Backspace || key == Keys::Delete) && focused_->hasSelection()) {
            eraseSelection(*focused_);
            text = focused_->value();
            caret = static_cast<std::size_t>(focused_->caret());
            changed = true;
        } else if (key == Keys::Backspace && caret > 0) {
            const std::size_t previous = previousUtf8(text, caret);
            text.erase(previous, caret - previous);
            caret = previous;
            changed = true;
        } else if (key == Keys::Delete && caret < text.size()) {
            const std::size_t next = nextUtf8(text, caret);
            text.erase(caret, next - caret);
            changed = true;
        } else if (key == Keys::Left || key == Keys::Right || key == Keys::Home ||
                   key == Keys::End) {
            const std::size_t previousCaret = caret;
            if (key == Keys::Left) {
                caret = previousUtf8(text, caret);
            } else if (key == Keys::Right) {
                caret = nextUtf8(text, caret);
            } else if (key == Keys::Home) {
                caret = 0;
            } else {
                caret = text.size();
            }
            if (shift) {
                if (!focused_->hasSelection()) {
                    selectionAnchor_ = static_cast<int>(previousCaret);
                }
                focused_->setSelection(selectionAnchor_, static_cast<int>(caret));
            } else {
                focused_->clearSelection();
            }
        }

        if (changed) {
            focused_->setValue(std::move(text));
            focused_->setCaret(static_cast<int>(caret));
            focused_->clearSelection();
            dispatchChange(*focused_);
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

    if (focused_->hasSelection()) {
        eraseSelection(*focused_);
    }
    std::string value = focused_->value();
    const int caret = std::clamp(focused_->caret(), 0, static_cast<int>(value.size()));
    value.insert(static_cast<std::size_t>(caret), std::string(text));
    focused_->setValue(std::move(value));
    focused_->setCaret(caret + static_cast<int>(text.size()));
    focused_->clearSelection();
    dispatchChange(*focused_);
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
