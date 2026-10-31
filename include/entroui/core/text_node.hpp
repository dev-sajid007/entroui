#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "entroui/core/node.hpp"

namespace EntroUI {

class TextNode : public Node {
public:
    explicit TextNode(std::string text);

    TextNode(TextNode&&) noexcept = default;
    TextNode& operator=(TextNode&&) noexcept = default;
    TextNode(const TextNode&) = delete;
    TextNode& operator=(const TextNode&) = delete;
    ~TextNode() override = default;

    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    void setText(std::string text);
    void setText(std::string_view text) { setText(std::string(text)); }

    [[nodiscard]] std::unique_ptr<Node> clone() const override;
    [[nodiscard]] std::unique_ptr<TextNode> cloneText() const;

private:
    std::string text_;
};

} // namespace EntroUI
