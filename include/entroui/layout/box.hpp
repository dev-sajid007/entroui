#pragma once

#include <algorithm>

namespace EntroUI {

struct Size {
    float width = 0.0F;
    float height = 0.0F;
};

struct Point {
    float x = 0.0F;
    float y = 0.0F;
};

struct Rect {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    [[nodiscard]] float right() const noexcept { return x + width; }
    [[nodiscard]] float bottom() const noexcept { return y + height; }
    [[nodiscard]] Point center() const noexcept { return {x + width * 0.5F, y + height * 0.5F}; }

    [[nodiscard]] bool empty() const noexcept { return width <= 0.0F || height <= 0.0F; }

    [[nodiscard]] bool contains(float px, float py) const noexcept {
        return px >= x && py >= y && px < x + width && py < y + height;
    }

    [[nodiscard]] bool intersects(const Rect& other) const noexcept {
        return x < other.right() && right() > other.x && y < other.bottom() &&
               bottom() > other.y;
    }

    [[nodiscard]] Rect intersection(const Rect& other) const noexcept {
        const float left = std::max(x, other.x);
        const float top = std::max(y, other.y);
        const float rightEdge = std::min(right(), other.right());
        const float bottomEdge = std::min(bottom(), other.bottom());
        if (rightEdge <= left || bottomEdge <= top) {
            return {};
        }
        return {left, top, rightEdge - left, bottomEdge - top};
    }

    [[nodiscard]] Rect inset(float top, float rightInset, float bottomInset,
                             float leftInset) const noexcept {
        return {x + leftInset, y + top, std::max(0.0F, width - leftInset - rightInset),
                std::max(0.0F, height - top - bottomInset)};
    }

    [[nodiscard]] Rect inset(float amount) const noexcept {
        return inset(amount, amount, amount, amount);
    }

    void translate(float dx, float dy) noexcept {
        x += dx;
        y += dy;
    }

    bool operator==(const Rect&) const = default;
};

} // namespace EntroUI
