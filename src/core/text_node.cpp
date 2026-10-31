#include "entroui/core/text_node.hpp"

#include "entroui/core/document.hpp"

namespace EntroUI {

TextNode::TextNode(std::string text) : Node(NodeType::Text), text_(std::move(text)) {}

void TextNode::setText(std::string text) {
    text_ = std::move(text);
    if (auto* document = ownerDocument()) {
        document->invalidateLayout();
    }
}

std::unique_ptr<Node> TextNode::clone() const { return cloneText(); }

std::unique_ptr<TextNode> TextNode::cloneText() const {
    return std::make_unique<TextNode>(text_);
}

} // namespace EntroUI
