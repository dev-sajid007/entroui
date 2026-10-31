#pragma once

namespace EntroUI {

struct Event;

class EventTarget {
public:
    virtual ~EventTarget() = default;
    virtual void dispatchEvent(Event& event) = 0;
};

} // namespace EntroUI
