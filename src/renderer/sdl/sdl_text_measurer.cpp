#include "renderer/sdl/sdl_text_measurer.hpp"

#include <string>

namespace EntroUI::detail {

TextMetrics SDLTextMeasurer::measure(std::string_view text, const FontSpec& fontSpec) {
    TTF_Font* font = fonts_.get(fontSpec);
    if (font == nullptr) {
        const float height = fontSpec.size * 1.2F;
        return {static_cast<float>(text.size()) * fontSpec.size * 0.5F, height};
    }

    int width = 0;
    int height = 0;
    if (!TTF_GetStringSize(font, text.data(), text.size(), &width, &height)) {
        return {static_cast<float>(text.size()) * fontSpec.size * 0.5F, fontSpec.size * 1.2F};
    }
    return {static_cast<float>(width), static_cast<float>(height)};
}

} // namespace EntroUI::detail
