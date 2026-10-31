#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace EntroUI {

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                    std::uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    static std::optional<Color> tryParse(std::string_view text) noexcept;
    static Color parse(std::string_view text);

    bool operator==(const Color&) const = default;

    [[nodiscard]] constexpr bool visible() const noexcept { return a != 0; }
    [[nodiscard]] constexpr std::uint32_t rgba() const noexcept {
        return (static_cast<std::uint32_t>(r) << 24U) | (static_cast<std::uint32_t>(g) << 16U) |
               (static_cast<std::uint32_t>(b) << 8U) | static_cast<std::uint32_t>(a);
    }
};

enum class LengthUnit { Auto, Px, Percent };

struct Length {
    float value = 0.0F;
    LengthUnit unit = LengthUnit::Auto;

    constexpr Length() = default;
    constexpr Length(float pixels, LengthUnit lengthUnit = LengthUnit::Px)
        : value(pixels), unit(lengthUnit) {}

    static std::optional<Length> tryParse(std::string_view text) noexcept;
    static Length parse(std::string_view text);

    [[nodiscard]] constexpr bool isAuto() const noexcept { return unit == LengthUnit::Auto; }
    [[nodiscard]] float resolve(float reference, float fallback = 0.0F) const noexcept {
        switch (unit) {
        case LengthUnit::Px:
            return value;
        case LengthUnit::Percent:
            return reference * (value / 100.0F);
        case LengthUnit::Auto:
        default:
            return fallback;
        }
    }
};

struct EdgeInsets {
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;
    float left = 0.0F;

    [[nodiscard]] float horizontal() const noexcept { return left + right; }
    [[nodiscard]] float vertical() const noexcept { return top + bottom; }
};

struct EdgeLengths {
    Length top;
    Length right;
    Length bottom;
    Length left;

    [[nodiscard]] EdgeInsets resolve(float reference) const noexcept {
        return {top.resolve(reference), right.resolve(reference), bottom.resolve(reference),
                left.resolve(reference)};
    }
};

enum class Display { Block, Inline, InlineBlock, Flex, None };
enum class FlexDirection { Row, Column };
enum class JustifyContent { Start, Center, End, SpaceBetween, SpaceAround };
enum class AlignItems { Start, Center, End, Stretch };
enum class AlignSelf { Auto, Start, Center, End, Stretch };
enum class AlignContent { Start, Center, End, SpaceBetween, SpaceAround, Stretch };
enum class FlexWrap { NoWrap, Wrap };
enum class TextAlign { Left, Center, Right };

struct FontSpec {
    std::string family;
    float size = 16.0F;
    int weight = 400;

    [[nodiscard]] bool bold() const noexcept { return weight >= 600; }
    bool operator==(const FontSpec&) const = default;
};

struct StyleProperties {
    Length width;
    Length height;
    Length minWidth;
    Length minHeight;
    Length maxWidth;
    Length maxHeight;

    EdgeLengths margin;
    EdgeLengths padding;

    float borderWidth = 0.0F;
    Color borderColor{0, 0, 0, 0};
    Color color{17, 24, 39, 255};
    Color background{0, 0, 0, 0};
    float borderRadius = 0.0F;

    float fontSize = 16.0F;
    int fontWeight = 400;
    std::string fontFamily;
    TextAlign textAlign = TextAlign::Left;
    float lineHeight = 0.0F;

    Display display = Display::Block;
    FlexDirection flexDirection = FlexDirection::Row;
    JustifyContent justifyContent = JustifyContent::Start;
    AlignItems alignItems = AlignItems::Stretch;
    AlignSelf alignSelf = AlignSelf::Auto;
    AlignContent alignContent = AlignContent::Stretch;
    FlexWrap flexWrap = FlexWrap::NoWrap;
    float gap = 0.0F;
    float rowGap = -1.0F;
    float columnGap = -1.0F;
    Length flexBasis;
    float flexGrow = 0.0F;
    float flexShrink = 1.0F;

    [[nodiscard]] float resolvedRowGap() const noexcept {
        return rowGap >= 0.0F ? rowGap : gap;
    }
    [[nodiscard]] float resolvedColumnGap() const noexcept {
        return columnGap >= 0.0F ? columnGap : gap;
    }

    [[nodiscard]] float resolvedLineHeight() const noexcept {
        return lineHeight > 0.0F ? lineHeight : fontSize * 1.2F;
    }
};

void applyDefaultStyle(StyleProperties& properties, std::string_view tag);
void inheritStyle(StyleProperties& properties, const StyleProperties& parent);

bool applyStyleDeclaration(StyleProperties& properties, std::string_view property,
                           std::string_view value);

bool applyInlineStyle(StyleProperties& properties, std::string_view declarations);

bool isValidStyleProperty(std::string_view property);

} // namespace EntroUI
