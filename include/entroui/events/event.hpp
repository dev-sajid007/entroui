#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace EntroUI {

class Element;

enum class EventType {
    Click,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput,
    Focus,
    Blur,
    Change,
};

struct Event {
    explicit Event(EventType eventType) : type(eventType) {}
    virtual ~Event() = default;

    EventType type;
    Element* target = nullptr;
    bool propagationStopped = false;
    bool defaultPrevented = false;

    void stopPropagation() noexcept { propagationStopped = true; }
    void preventDefault() noexcept { defaultPrevented = true; }
};

struct MouseEvent : Event {
    MouseEvent(EventType eventType, float mouseX, float mouseY)
        : Event(eventType), x(mouseX), y(mouseY) {}

    float x = 0.0F;
    float y = 0.0F;
    int button = 0;
    int clicks = 1;
};

struct WheelEvent : Event {
    WheelEvent(float mouseX, float mouseY, float scrollX, float scrollY)
        : Event(EventType::MouseWheel), x(mouseX), y(mouseY), deltaX(scrollX), deltaY(scrollY) {}

    float x = 0.0F;
    float y = 0.0F;
    float deltaX = 0.0F;
    float deltaY = 0.0F;
};

struct KeyEvent : Event {
    KeyEvent(EventType eventType, int keyCode, int scanCode, int modifiers)
        : Event(eventType), key(keyCode), scancode(scanCode), mods(modifiers) {}

    int key = 0;
    int scancode = 0;
    int mods = 0;
    bool repeat = false;
};

struct TextInputEvent : Event {
    explicit TextInputEvent(std::string input)
        : Event(EventType::TextInput), text(std::move(input)) {}

    std::string text;
};

struct FocusEvent : Event {
    explicit FocusEvent(EventType eventType) : Event(eventType) {}
};

namespace Keys {
inline constexpr int Backspace = 8;
inline constexpr int Tab = 9;
inline constexpr int Enter = 13;
inline constexpr int Escape = 27;
inline constexpr int Space = 32;
inline constexpr int Delete = 127;
inline constexpr int Left = 0x1001;
inline constexpr int Right = 0x1002;
inline constexpr int Up = 0x1003;
inline constexpr int Down = 0x1004;
inline constexpr int Home = 0x1005;
inline constexpr int End = 0x1006;
} // namespace Keys

using EventCallback = std::function<void(Event&)>;

} // namespace EntroUI
