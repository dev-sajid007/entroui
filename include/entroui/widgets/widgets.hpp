#pragma once

#include <string>

#include "entroui/core/element.hpp"

namespace EntroUI::Widgets {

[[nodiscard]] Element button(std::string text = "Button");
[[nodiscard]] Element label(std::string text);
[[nodiscard]] Element textInput(std::string placeholder = {}, std::string value = {});
[[nodiscard]] Element checkbox(std::string label, bool checked = false);
[[nodiscard]] Element image(std::string source);
[[nodiscard]] Element container();
[[nodiscard]] Element scrollView(Element&& content, float height = 240.0F);

} // namespace EntroUI::Widgets
