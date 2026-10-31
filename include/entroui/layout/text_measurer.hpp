#pragma once

#include <optional>
#include <string_view>

#include "entroui/layout/box.hpp"
#include "entroui/style/style.hpp"

namespace EntroUI {

struct TextMetrics {
    float width = 0.0F;
    float height = 0.0F;
};

class TextMeasurer {
public:
    virtual ~TextMeasurer() = default;
    [[nodiscard]] virtual TextMetrics measure(std::string_view text, const FontSpec& font) = 0;
};

class FixedTextMeasurer : public TextMeasurer {
public:
    [[nodiscard]] TextMetrics measure(std::string_view text, const FontSpec& font) override {
        const float height = font.size * 1.2F;
        return {static_cast<float>(text.size()) * font.size * 0.5F, height};
    }
};

class ImageSizeProvider {
public:
    virtual ~ImageSizeProvider() = default;
    [[nodiscard]] virtual std::optional<Size> imageSize(std::string_view source) const = 0;
};

} // namespace EntroUI
