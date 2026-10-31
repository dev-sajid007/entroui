#pragma once

#include <string>
#include <variant>
#include <vector>

#include "entroui/layout/box.hpp"
#include "entroui/style/style.hpp"

namespace EntroUI {

struct RectCommand {
    Rect rect;
    Color color;
    float radius = 0.0F;
};

struct BorderCommand {
    Rect rect;
    Color color;
    float width = 1.0F;
    float radius = 0.0F;
};

struct TextCommand {
    Rect rect;
    std::string text;
    FontSpec font;
    Color color;
    TextAlign align = TextAlign::Left;
    float lineHeight = 0.0F;
};

struct ImageCommand {
    Rect rect;
    std::string source;
    float radius = 0.0F;
};

struct PushClipCommand {
    Rect rect;
};

struct PopClipCommand {};

using RenderCommand =
    std::variant<RectCommand, BorderCommand, TextCommand, ImageCommand, PushClipCommand,
                 PopClipCommand>;

class RenderCommandList {
public:
    void add(RenderCommand command) { commands_.push_back(std::move(command)); }
    void clear() { commands_.clear(); }

    [[nodiscard]] const std::vector<RenderCommand>& commands() const noexcept {
        return commands_;
    }
    [[nodiscard]] std::vector<RenderCommand>& commands() noexcept { return commands_; }
    [[nodiscard]] std::size_t size() const noexcept { return commands_.size(); }
    [[nodiscard]] bool empty() const noexcept { return commands_.empty(); }

private:
    std::vector<RenderCommand> commands_;
};

} // namespace EntroUI
