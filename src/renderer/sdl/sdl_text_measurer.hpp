#pragma once

#include "entroui/layout/text_measurer.hpp"
#include "renderer/sdl/sdl_font.hpp"

namespace EntroUI::detail {

class SDLTextMeasurer : public TextMeasurer {
public:
    explicit SDLTextMeasurer(FontCache& fonts) : fonts_(fonts) {}

    [[nodiscard]] TextMetrics measure(std::string_view text, const FontSpec& font) override;

private:
    FontCache& fonts_;
};

} // namespace EntroUI::detail
