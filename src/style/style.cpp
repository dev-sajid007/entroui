#include "entroui/style/style.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace EntroUI {

namespace {

std::string trimCopy(std::string_view value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return std::string(value.substr(begin, end - begin));
}

std::string lowerCopy(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

std::vector<std::string> splitWhitespace(std::string_view value) {
    std::vector<std::string> tokens;
    std::size_t position = 0;
    while (position < value.size()) {
        while (position < value.size() &&
               std::isspace(static_cast<unsigned char>(value[position])) != 0) {
            ++position;
        }
        std::size_t end = position;
        while (end < value.size() &&
               std::isspace(static_cast<unsigned char>(value[end])) == 0) {
            ++end;
        }
        if (end > position) {
            tokens.emplace_back(value.substr(position, end - position));
        }
        position = end;
    }
    return tokens;
}

int hexValue(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    return -1;
}

std::optional<std::uint8_t> parseChannel(std::string_view text) {
    std::string value = trimCopy(text);
    if (value.empty()) {
        return std::nullopt;
    }
    if (value.back() == '%') {
        value.pop_back();
        char* end = nullptr;
        const double percent = std::strtod(value.c_str(), &end);
        if (end == nullptr || *end != '\0') {
            return std::nullopt;
        }
        const double clamped = std::clamp(percent, 0.0, 100.0);
        return static_cast<std::uint8_t>(std::lround(clamped * 255.0 / 100.0));
    }
    char* end = nullptr;
    const long number = std::strtol(value.c_str(), &end, 10);
    if (end == nullptr || *end != '\0') {
        return std::nullopt;
    }
    return static_cast<std::uint8_t>(std::clamp(number, 0L, 255L));
}

std::optional<std::uint8_t> parseAlpha(std::string_view text) {
    std::string value = trimCopy(text);
    if (value.empty()) {
        return std::nullopt;
    }
    if (value.back() == '%') {
        value.pop_back();
    }
    char* end = nullptr;
    const double number = std::strtod(value.c_str(), &end);
    if (end == nullptr || *end != '\0') {
        return std::nullopt;
    }
    if (number <= 1.0 && (value.find('.') != std::string::npos || number == 0.0)) {
        return static_cast<std::uint8_t>(std::lround(std::clamp(number, 0.0, 1.0) * 255.0));
    }
    return static_cast<std::uint8_t>(std::lround(std::clamp(number, 0.0, 255.0)));
}

std::vector<std::string> splitArguments(std::string_view value) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= value.size()) {
        std::size_t comma = value.find(',', start);
        if (comma == std::string_view::npos) {
            comma = value.size();
        }
        parts.push_back(trimCopy(value.substr(start, comma - start)));
        if (comma == value.size()) {
            break;
        }
        start = comma + 1;
    }
    return parts;
}

void applyEdgeShorthand(EdgeLengths& edges, std::string_view value) {
    const auto tokens = splitWhitespace(value);
    if (tokens.empty() || tokens.size() > 4) {
        return;
    }
    std::array<Length, 4> parsed{};
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        const auto length = Length::tryParse(tokens[index]);
        if (!length) {
            return;
        }
        parsed[index] = *length;
    }
    switch (tokens.size()) {
    case 1:
        edges = {parsed[0], parsed[0], parsed[0], parsed[0]};
        break;
    case 2:
        edges = {parsed[0], parsed[1], parsed[0], parsed[1]};
        break;
    case 3:
        edges = {parsed[0], parsed[1], parsed[2], parsed[1]};
        break;
    case 4:
    default:
        edges = {parsed[0], parsed[1], parsed[2], parsed[3]};
        break;
    }
}

std::optional<float> parseFloat(std::string_view value) {
    std::string text = trimCopy(value);
    if (text.size() > 2) {
        const std::string suffix = lowerCopy(std::string_view(text).substr(text.size() - 2));
        if (suffix == "px") {
            text = trimCopy(std::string_view(text).substr(0, text.size() - 2));
        }
    }
    char* end = nullptr;
    const float number = std::strtof(text.c_str(), &end);
    if (end == nullptr || *end != '\0') {
        return std::nullopt;
    }
    return number;
}

std::optional<Length> parseLengthValue(std::string_view value) {
    return Length::tryParse(value);
}

bool parseDisplay(std::string_view value, Display& display) {
    const std::string lower = lowerCopy(trimCopy(value));
    if (lower == "block") {
        display = Display::Block;
    } else if (lower == "flex") {
        display = Display::Flex;
    } else if (lower == "none") {
        display = Display::None;
    } else if (lower == "inline-block" || lower == "inline") {
        display = Display::InlineBlock;
    } else {
        return false;
    }
    return true;
}

bool parseFlexDirection(std::string_view value, FlexDirection& direction) {
    const std::string lower = lowerCopy(trimCopy(value));
    if (lower == "row") {
        direction = FlexDirection::Row;
    } else if (lower == "column") {
        direction = FlexDirection::Column;
    } else {
        return false;
    }
    return true;
}

bool parseJustify(std::string_view value, JustifyContent& justify) {
    const std::string lower = lowerCopy(trimCopy(value));
    if (lower == "flex-start" || lower == "start") {
        justify = JustifyContent::Start;
    } else if (lower == "center") {
        justify = JustifyContent::Center;
    } else if (lower == "flex-end" || lower == "end") {
        justify = JustifyContent::End;
    } else if (lower == "space-between") {
        justify = JustifyContent::SpaceBetween;
    } else if (lower == "space-around") {
        justify = JustifyContent::SpaceAround;
    } else {
        return false;
    }
    return true;
}

bool parseAlign(std::string_view value, AlignItems& align) {
    const std::string lower = lowerCopy(trimCopy(value));
    if (lower == "flex-start" || lower == "start") {
        align = AlignItems::Start;
    } else if (lower == "center") {
        align = AlignItems::Center;
    } else if (lower == "flex-end" || lower == "end") {
        align = AlignItems::End;
    } else if (lower == "stretch") {
        align = AlignItems::Stretch;
    } else {
        return false;
    }
    return true;
}

bool parseTextAlign(std::string_view value, TextAlign& align) {
    const std::string lower = lowerCopy(trimCopy(value));
    if (lower == "left" || lower == "start") {
        align = TextAlign::Left;
    } else if (lower == "center") {
        align = TextAlign::Center;
    } else if (lower == "right" || lower == "end") {
        align = TextAlign::Right;
    } else {
        return false;
    }
    return true;
}

bool parseBorderShorthand(StyleProperties& properties, std::string_view value) {
    for (const auto& token : splitWhitespace(value)) {
        if (auto length = Length::tryParse(token); length && !length->isAuto()) {
            properties.borderWidth = std::max(0.0F, length->value);
        } else if (auto color = Color::tryParse(token)) {
            properties.borderColor = *color;
        }
    }
    return true;
}

} // namespace

std::optional<Color> Color::tryParse(std::string_view text) noexcept {
    const std::string value = trimCopy(text);
    if (value.empty()) {
        return std::nullopt;
    }
    if (value.front() == '#') {
        const std::string_view hex = value.substr(1);
        std::vector<int> digits;
        digits.reserve(hex.size());
        for (char character : hex) {
            const int digit = hexValue(character);
            if (digit < 0) {
                return std::nullopt;
            }
            digits.push_back(digit);
        }
        if (digits.size() == 3 || digits.size() == 4) {
            return Color{static_cast<std::uint8_t>(digits[0] * 17),
                         static_cast<std::uint8_t>(digits[1] * 17),
                         static_cast<std::uint8_t>(digits[2] * 17),
                         static_cast<std::uint8_t>(digits.size() == 4 ? digits[3] * 17 : 255)};
        }
        if (digits.size() == 6 || digits.size() == 8) {
            return Color{static_cast<std::uint8_t>(digits[0] * 16 + digits[1]),
                         static_cast<std::uint8_t>(digits[2] * 16 + digits[3]),
                         static_cast<std::uint8_t>(digits[4] * 16 + digits[5]),
                         static_cast<std::uint8_t>(digits.size() == 8
                                                       ? digits[6] * 16 + digits[7]
                                                       : 255)};
        }
        return std::nullopt;
    }

    const std::string lower = lowerCopy(value);
    if (lower.rfind("rgb(", 0) == 0 || lower.rfind("rgba(", 0) == 0) {
        const std::size_t open = lower.find('(');
        const std::size_t close = lower.rfind(')');
        if (close == std::string::npos || close <= open) {
            return std::nullopt;
        }
        const auto parts = splitArguments(std::string_view(value).substr(open + 1, close - open - 1));
        if (parts.size() != 3 && parts.size() != 4) {
            return std::nullopt;
        }
        const auto red = parseChannel(parts[0]);
        const auto green = parseChannel(parts[1]);
        const auto blue = parseChannel(parts[2]);
        if (!red || !green || !blue) {
            return std::nullopt;
        }
        std::uint8_t alpha = 255;
        if (parts.size() == 4) {
            const auto parsedAlpha = parseAlpha(parts[3]);
            if (!parsedAlpha) {
                return std::nullopt;
            }
            alpha = *parsedAlpha;
        }
        return Color{*red, *green, *blue, alpha};
    }

    static const std::unordered_map<std::string, Color> named = {
        {"transparent", Color{0, 0, 0, 0}}, {"black", Color{0, 0, 0, 255}},
        {"white", Color{255, 255, 255, 255}}, {"red", Color{255, 0, 0, 255}},
        {"green", Color{0, 128, 0, 255}},     {"blue", Color{0, 0, 255, 255}},
        {"gray", Color{128, 128, 128, 255}},  {"grey", Color{128, 128, 128, 255}},
        {"lightgray", Color{211, 211, 211, 255}},
        {"lightgrey", Color{211, 211, 211, 255}},
        {"darkgray", Color{169, 169, 169, 255}},
        {"darkgrey", Color{169, 169, 169, 255}},
        {"yellow", Color{255, 255, 0, 255}},  {"orange", Color{255, 165, 0, 255}},
        {"purple", Color{128, 0, 128, 255}},  {"pink", Color{255, 192, 203, 255}},
        {"teal", Color{0, 128, 128, 255}},    {"navy", Color{0, 0, 128, 255}},
        {"silver", Color{192, 192, 192, 255}}, {"maroon", Color{128, 0, 0, 255}},
        {"lime", Color{0, 255, 0, 255}},      {"aqua", Color{0, 255, 255, 255}},
        {"cyan", Color{0, 255, 255, 255}},    {"magenta", Color{255, 0, 255, 255}},
        {"gold", Color{255, 215, 0, 255}},    {"coral", Color{255, 127, 80, 255}},
        {"tomato", Color{255, 99, 71, 255}},  {"indigo", Color{75, 0, 130, 255}},
        {"violet", Color{238, 130, 238, 255}}, {"brown", Color{165, 42, 42, 255}},
    };
    const auto it = named.find(lower);
    if (it != named.end()) {
        return it->second;
    }
    return std::nullopt;
}

Color Color::parse(std::string_view text) {
    const auto color = tryParse(text);
    if (!color) {
        throw std::invalid_argument("Invalid color: " + std::string(text));
    }
    return *color;
}

std::optional<Length> Length::tryParse(std::string_view text) noexcept {
    const std::string value = trimCopy(text);
    if (value.empty()) {
        return std::nullopt;
    }
    if (lowerCopy(value) == "auto") {
        return Length{0.0F, LengthUnit::Auto};
    }
    if (value.back() == '%') {
        const auto number = parseFloat(std::string_view(value).substr(0, value.size() - 1));
        if (!number) {
            return std::nullopt;
        }
        return Length{*number, LengthUnit::Percent};
    }
    if (value.size() > 2) {
        const std::string suffix = lowerCopy(std::string_view(value).substr(value.size() - 2));
        if (suffix == "px") {
            const auto number = parseFloat(std::string_view(value).substr(0, value.size() - 2));
            if (!number) {
                return std::nullopt;
            }
            return Length{*number, LengthUnit::Px};
        }
    }
    const auto number = parseFloat(value);
    if (!number) {
        return std::nullopt;
    }
    return Length{*number, LengthUnit::Px};
}

Length Length::parse(std::string_view text) {
    const auto length = tryParse(text);
    if (!length) {
        throw std::invalid_argument("Invalid length: " + std::string(text));
    }
    return *length;
}

void applyDefaultStyle(StyleProperties& properties, std::string_view tag) {
    properties.display = Display::Block;
    if (tag == "span" || tag == "button" || tag == "input" || tag == "label" ||
        tag == "image") {
        properties.display = Display::InlineBlock;
    }
    if (tag == "h1") {
        properties.fontSize = 32.0F;
        properties.fontWeight = 700;
    } else if (tag == "h2") {
        properties.fontSize = 24.0F;
        properties.fontWeight = 700;
    }
}

void inheritStyle(StyleProperties& properties, const StyleProperties& parent) {
    properties.color = parent.color;
    properties.fontSize = parent.fontSize;
    properties.fontWeight = parent.fontWeight;
    properties.fontFamily = parent.fontFamily;
    properties.textAlign = parent.textAlign;
    properties.lineHeight = parent.lineHeight;
}

bool applyStyleDeclaration(StyleProperties& properties, std::string_view property,
                           std::string_view value) {
    const std::string name = lowerCopy(trimCopy(property));
    const std::string rawValue = trimCopy(value);
    const std::string lowerValue = lowerCopy(rawValue);

    if (name == "width" || name == "height" || name == "min-width" || name == "min-height" ||
        name == "max-width" || name == "max-height") {
        const auto length = parseLengthValue(rawValue);
        if (!length) {
            return false;
        }
        if (name == "width") {
            properties.width = *length;
        } else if (name == "height") {
            properties.height = *length;
        } else if (name == "min-width") {
            properties.minWidth = *length;
        } else if (name == "min-height") {
            properties.minHeight = *length;
        } else if (name == "max-width") {
            properties.maxWidth = *length;
        } else {
            properties.maxHeight = *length;
        }
        return true;
    }
    if (name == "margin" || name == "padding") {
        auto& target = name == "margin" ? properties.margin : properties.padding;
        applyEdgeShorthand(target, rawValue);
        return true;
    }
    if (name == "margin-top" || name == "margin-right" || name == "margin-bottom" ||
        name == "margin-left") {
        const auto length = parseLengthValue(rawValue);
        if (!length) {
            return false;
        }
        if (name == "margin-top") {
            properties.margin.top = *length;
        } else if (name == "margin-right") {
            properties.margin.right = *length;
        } else if (name == "margin-bottom") {
            properties.margin.bottom = *length;
        } else {
            properties.margin.left = *length;
        }
        return true;
    }
    if (name == "padding-top" || name == "padding-right" || name == "padding-bottom" ||
        name == "padding-left") {
        const auto length = parseLengthValue(rawValue);
        if (!length) {
            return false;
        }
        if (name == "padding-top") {
            properties.padding.top = *length;
        } else if (name == "padding-right") {
            properties.padding.right = *length;
        } else if (name == "padding-bottom") {
            properties.padding.bottom = *length;
        } else {
            properties.padding.left = *length;
        }
        return true;
    }
    if (name == "color" || name == "background" || name == "background-color" ||
        name == "border-color") {
        const auto color = Color::tryParse(rawValue);
        if (!color) {
            return false;
        }
        if (name == "color") {
            properties.color = *color;
        } else if (name == "border-color") {
            properties.borderColor = *color;
        } else {
            properties.background = *color;
        }
        return true;
    }
    if (name == "border") {
        return parseBorderShorthand(properties, rawValue);
    }
    if (name == "border-width" || name == "border-radius" || name == "gap") {
        const auto number = parseFloat(rawValue);
        if (!number) {
            return false;
        }
        if (name == "border-width") {
            properties.borderWidth = std::max(0.0F, *number);
        } else if (name == "border-radius") {
            properties.borderRadius = std::max(0.0F, *number);
        } else {
            properties.gap = std::max(0.0F, *number);
        }
        return true;
    }
    if (name == "font-size") {
        const auto number = parseFloat(rawValue);
        if (!number || *number <= 0.0F) {
            return false;
        }
        properties.fontSize = *number;
        return true;
    }
    if (name == "font-weight") {
        if (lowerValue == "normal") {
            properties.fontWeight = 400;
        } else if (lowerValue == "bold") {
            properties.fontWeight = 700;
        } else if (lowerValue == "lighter") {
            properties.fontWeight = 300;
        } else if (lowerValue == "bolder") {
            properties.fontWeight = 700;
        } else {
            const auto number = parseFloat(rawValue);
            if (!number || *number < 1.0F || *number > 1000.0F) {
                return false;
            }
            properties.fontWeight = static_cast<int>(*number);
        }
        return true;
    }
    if (name == "font-family") {
        properties.fontFamily = rawValue;
        return true;
    }
    if (name == "text-align") {
        return parseTextAlign(rawValue, properties.textAlign);
    }
    if (name == "line-height") {
        if (lowerValue == "normal") {
            properties.lineHeight = 0.0F;
            return true;
        }
        const auto number = parseFloat(rawValue);
        if (!number || *number <= 0.0F) {
            return false;
        }
        properties.lineHeight = rawValue.find("px") != std::string::npos ||
                                        rawValue.find("PX") != std::string::npos
                                    ? *number
                                    : *number * properties.fontSize;
        return true;
    }
    if (name == "display") {
        return parseDisplay(rawValue, properties.display);
    }
    if (name == "flex-direction") {
        return parseFlexDirection(rawValue, properties.flexDirection);
    }
    if (name == "justify-content") {
        return parseJustify(rawValue, properties.justifyContent);
    }
    if (name == "align-items") {
        return parseAlign(rawValue, properties.alignItems);
    }
    return false;
}

bool applyInlineStyle(StyleProperties& properties, std::string_view declarations) {
    bool applied = false;
    std::size_t start = 0;
    while (start < declarations.size()) {
        std::size_t end = declarations.find(';', start);
        if (end == std::string_view::npos) {
            end = declarations.size();
        }
        const std::string_view declaration = declarations.substr(start, end - start);
        const std::size_t colon = declaration.find(':');
        if (colon != std::string_view::npos) {
            const std::string property = trimCopy(declaration.substr(0, colon));
            std::string value = trimCopy(declaration.substr(colon + 1));
            if (!value.empty() && value.back() == '!') {
                value.pop_back();
            }
            if (!property.empty() && applyStyleDeclaration(properties, property, value)) {
                applied = true;
            }
        }
        start = end + 1;
    }
    return applied;
}

bool isValidStyleProperty(std::string_view property) {
    const std::string name = lowerCopy(trimCopy(property));
    static const std::array<const char*, 33> properties = {
        "width",           "height",         "min-width",       "min-height",
        "max-width",       "max-height",     "margin",          "margin-top",
        "margin-right",    "margin-bottom",  "margin-left",     "padding",
        "padding-top",     "padding-right",  "padding-bottom",  "padding-left",
        "color",           "background",     "background-color", "border-color",
        "border",          "border-width",   "border-radius",   "font-size",
        "font-weight",     "font-family",    "text-align",      "line-height",
        "display",         "flex-direction", "justify-content", "align-items",
        "gap",
    };
    return std::find_if(properties.begin(), properties.end(), [&name](const char* candidate) {
               return name == candidate;
           }) != properties.end();
}

} // namespace EntroUI
