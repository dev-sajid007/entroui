#include "entroui/core/node.hpp"

#include "entroui/core/document.hpp"

namespace EntroUI {

Node::Node(NodeType type) : type_(type) {}

Node::Node(Node&& other) noexcept
    : type_(other.type_),
      parent_(other.parent_),
      document_(other.document_),
      uid_(other.uid_) {
    other.parent_ = nullptr;
    other.document_ = nullptr;
    other.uid_ = 0;
}

Node& Node::operator=(Node&& other) noexcept {
    if (this != &other) {
        type_ = other.type_;
        parent_ = other.parent_;
        document_ = other.document_;
        uid_ = other.uid_;
        other.parent_ = nullptr;
        other.document_ = nullptr;
        other.uid_ = 0;
    }
    return *this;
}

Node::~Node() = default;

void Node::attachTo(Document* document) {
    document_ = document;
    uid_ = document != nullptr ? document->nextUid() : 0;
}

void Node::detachFrom(Document* document) {
    if (document_ == document) {
        document_ = nullptr;
        uid_ = 0;
    }
}

} // namespace EntroUI
