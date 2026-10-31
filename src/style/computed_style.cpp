#include "entroui/style/computed_style.hpp"

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#include "entroui/core/document.hpp"
#include "entroui/core/element.hpp"
#include "entroui/style/stylesheet.hpp"

namespace EntroUI {

namespace {

constexpr std::size_t kUserOrderOffset = 1000000U;

const char* const kThemeCss = R"CSS(
button {
    padding: 10px 16px;
    background: #2563eb;
    color: #ffffff;
    border-radius: 6px;
    text-align: center;
}
button.hover {
    background: #3b82f6;
}
button.active {
    background: #1d4ed8;
}
input {
    padding: 10px 12px;
    background: #ffffff;
    color: #111827;
    border: 1px solid #d1d5db;
    border-radius: 6px;
    min-height: 42px;
    width: 240px;
}
input.focus {
    border-color: #2563eb;
}
label {
    color: #111827;
}
.eui-checkbox-box {
    width: 22px;
    height: 22px;
    padding: 0;
    background: #ffffff;
    border: 1px solid #9ca3af;
    border-radius: 4px;
    min-height: 0;
}
.eui-checkbox-box.checked {
    background: #2563eb;
    border-color: #2563eb;
}
.eui-checkbox-label {
    padding: 4px 0 0 8px;
}
.eui-scroll {
    padding: 0;
    border: 1px solid #e5e7eb;
    background: #ffffff;
    border-radius: 6px;
}
.eui-scroll-content {
    padding: 8px;
}
)CSS";

constexpr std::array<const char*, 42> kPropertyOrder = {
    "font-size",       "font-weight",    "font-family",     "line-height",
    "color",           "background",     "background-color", "border-color",
    "border",          "border-width",   "border-radius",   "text-align",
    "width",           "height",         "min-width",       "min-height",
    "max-width",       "max-height",     "margin",          "margin-top",
    "margin-right",    "margin-bottom",  "margin-left",     "padding",
    "padding-top",     "padding-right",  "padding-bottom",  "padding-left",
    "display",         "flex",           "flex-grow",       "flex-shrink",
    "flex-basis",      "flex-direction", "flex-wrap",       "justify-content",
    "align-items",     "align-self",     "align-content",   "gap",
    "row-gap",         "column-gap",
};

void computeElementStyle(Element& element, const StyleProperties* parentStyle,
                         const StyleSheet* theme, const StyleSheet* user) {
    StyleProperties properties;
    if (parentStyle != nullptr) {
        inheritStyle(properties, *parentStyle);
    }
    applyDefaultStyle(properties, element.tag());

    std::unordered_map<std::string, CascadedValue> cascade;
    if (theme != nullptr) {
        theme->collectMatching(element, cascade, 0);
    }
    if (user != nullptr) {
        user->collectMatching(element, cascade, kUserOrderOffset);
    }

    for (const char* property : kPropertyOrder) {
        const auto it = cascade.find(property);
        if (it != cascade.end()) {
            applyStyleDeclaration(properties, property, it->second.value);
        }
    }

    for (const auto& [property, value] : element.inlineStyles()) {
        if (!isValidStyleProperty(property)) {
            continue;
        }
        applyStyleDeclaration(properties, property, value);
    }

    if (properties.fontSize < 1.0F) {
        properties.fontSize = 1.0F;
    }
    if (properties.borderWidth < 0.0F) {
        properties.borderWidth = 0.0F;
    }
    if (properties.borderRadius < 0.0F) {
        properties.borderRadius = 0.0F;
    }
    if (properties.gap < 0.0F) {
        properties.gap = 0.0F;
    }

    element.mutableComputedStyle() = properties;
    element.setHasComputedStyle(true);

    for (auto& child : element.children()) {
        if (child->isElement()) {
            computeElementStyle(static_cast<Element&>(*child), &element.computedStyle(), theme,
                                user);
        }
    }
}

void computeAll(Document& document, const StyleSheet* theme, const StyleSheet* user) {
    if (!document.hasRoot()) {
        return;
    }
    computeElementStyle(document.root(), nullptr, theme, user);
    document.clearStyleDirty();
}

} // namespace

std::shared_ptr<const StyleSheet> defaultThemeStylesheet() {
    static const std::shared_ptr<const StyleSheet> theme = [] {
        auto parsed = StyleSheet::parse(kThemeCss, "<default-theme>");
        if (parsed.ok()) {
            return std::make_shared<StyleSheet>(std::move(*parsed.value));
        }
        return std::make_shared<StyleSheet>();
    }();
    return theme;
}

void computeDocumentStyles(Document& document, const StyleSheet* userStylesheet) {
    computeAll(document, nullptr, userStylesheet);
}

void computeDocumentStyles(Document& document, const StyleSheet* themeStylesheet,
                           const StyleSheet* userStylesheet) {
    computeAll(document, themeStylesheet, userStylesheet);
}

} // namespace EntroUI
