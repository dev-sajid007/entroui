#include "entroui/renderer/renderer.hpp"

#include <type_traits>
#include <variant>

namespace EntroUI {

void executeRenderCommands(Renderer& renderer, const RenderCommandList& commands) {
    for (const auto& command : commands.commands()) {
        std::visit(
            [&renderer](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, RectCommand>) {
                    renderer.drawRect(value.rect, value.color, value.radius);
                } else if constexpr (std::is_same_v<T, BorderCommand>) {
                    renderer.drawBorder(value.rect, value.color, value.width, value.radius);
                } else if constexpr (std::is_same_v<T, TextCommand>) {
                    renderer.drawText(value);
                } else if constexpr (std::is_same_v<T, ImageCommand>) {
                    renderer.drawImage(value);
                } else if constexpr (std::is_same_v<T, PushClipCommand>) {
                    renderer.pushClip(value.rect);
                } else if constexpr (std::is_same_v<T, PopClipCommand>) {
                    renderer.popClip();
                }
            },
            command);
    }
}

} // namespace EntroUI
