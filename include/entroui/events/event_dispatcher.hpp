#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "entroui/events/event.hpp"

namespace EntroUI {

class Document;
class Element;
class LayoutTree;

class EventDispatcher {
public:
    EventDispatcher(Document& document, LayoutTree& layout);

    [[nodiscard]] Element* hitTest(float x, float y) const;
    [[nodiscard]] Element* hovered() const noexcept { return hovered_; }
    [[nodiscard]] Element* focused() const noexcept { return focused_; }
    [[nodiscard]] Element* pressed() const noexcept { return pressed_; }

    void setFocus(Element* element);
    void clearInteraction();

    void handleMouseMove(float x, float y);
    void handleMouseDown(float x, float y, int button);
    void handleMouseUp(float x, float y, int button);
    void handleWheel(float x, float y, float deltaX, float deltaY);
    void handleKeyDown(int key, int scancode, int mods, bool repeat = false);
    void handleKeyUp(int key, int scancode, int mods);
    void handleTextInput(std::string_view text);

    void dispatch(Event& event);

private:
    void updateHover(Element* target);
    void updateHoverClasses(const std::vector<Element*>& path, bool hovered);
    void dispatchMouse(EventType type, Element* target, float x, float y, int button = 0,
                       int clicks = 1);
    void triggerClick(Element* target, float x, float y, int clicks);
    void focusNext(bool reverse);
    [[nodiscard]] std::vector<Element*> pathToRoot(Element* element) const;
    [[nodiscard]] std::vector<Element*> focusableElements() const;
    void applyDefaultAction(Event& event);
    [[nodiscard]] Element* scrollableAncestor(Element* element) const;
    [[nodiscard]] bool isFocusable(const Element& element) const;

    Document& document_;
    LayoutTree& layout_;
    Element* hovered_ = nullptr;
    Element* pressed_ = nullptr;
    Element* focused_ = nullptr;
    std::vector<Element*> hoverPath_;
    std::chrono::steady_clock::time_point lastClickTime_{};
    Element* lastClickTarget_ = nullptr;
    int clickCount_ = 0;
};

} // namespace EntroUI
