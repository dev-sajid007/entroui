#pragma once

#include <memory>

#include "entroui/style/style.hpp"

namespace EntroUI {

class Document;
class StyleSheet;

void computeDocumentStyles(Document& document, const StyleSheet* userStylesheet = nullptr);

void computeDocumentStyles(Document& document, const StyleSheet* themeStylesheet,
                           const StyleSheet* userStylesheet);

[[nodiscard]] std::shared_ptr<const StyleSheet> defaultThemeStylesheet();

} // namespace EntroUI
