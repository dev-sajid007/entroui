#pragma once

#include <cstdint>
#include <memory>

namespace EntroUI {

class Document;

enum class NodeType { Element, Text };

class Node {
public:
    virtual ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    [[nodiscard]] NodeType type() const noexcept { return type_; }
    [[nodiscard]] bool isElement() const noexcept { return type_ == NodeType::Element; }
    [[nodiscard]] bool isText() const noexcept { return type_ == NodeType::Text; }

    [[nodiscard]] Node* parent() const noexcept { return parent_; }
    [[nodiscard]] Document* ownerDocument() const noexcept { return document_; }
    [[nodiscard]] std::uint64_t uid() const noexcept { return uid_; }

    [[nodiscard]] virtual std::unique_ptr<Node> clone() const = 0;

protected:
    explicit Node(NodeType type);
    Node(Node&& other) noexcept;
    Node& operator=(Node&& other) noexcept;

    void setParentInternal(Node* parent) noexcept { parent_ = parent; }
    void setDocumentInternal(Document* document) noexcept { document_ = document; }
    void setUidInternal(std::uint64_t uid) noexcept { uid_ = uid; }

    virtual void attachTo(Document* document);
    virtual void detachFrom(Document* document);

private:
    friend class Element;
    friend class Document;

    NodeType type_;
    Node* parent_ = nullptr;
    Document* document_ = nullptr;
    std::uint64_t uid_ = 0;
};

} // namespace EntroUI
