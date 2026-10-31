#include "entroui/core/element.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

#include "entroui/core/document.hpp"
#include "entroui/core/text_node.hpp"

namespace EntroUI {

namespace {

std::string trim(std::string_view value) {
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

std::vector<std::string> splitClasses(std::string_view value) {
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start < value.size()) {
        while (start < value.size() &&
               std::isspace(static_cast<unsigned char>(value[start])) != 0) {
            ++start;
        }
        std::size_t end = start;
        while (end < value.size() &&
               std::isspace(static_cast<unsigned char>(value[end])) == 0) {
            ++end;
        }
        if (end > start) {
            result.emplace_back(value.substr(start, end - start));
        }
        start = end;
    }
    return result;
}

bool isTruthy(std::string_view value) {
    return value == "true" || value == "checked" || value == "1" || value == "yes";
}

} // namespace

Element::Element(std::string tag) : Node(NodeType::Element), tag_(std::move(tag)) {
    if (tag_ == "input" || tag_ == "button") {
        focusable_ = true;
    }
    if (tag_ == "input") {
        attributes_["type"] = "text";
    }
}

Element::Element(Element&& other) noexcept : Node(NodeType::Element) {
    tag_ = std::move(other.tag_);
    id_ = std::move(other.id_);
    classes_ = std::move(other.classes_);
    attributes_ = std::move(other.attributes_);
    inlineStyles_ = std::move(other.inlineStyles_);
    children_ = std::move(other.children_);
    listeners_ = std::move(other.listeners_);
    bindings_ = std::move(other.bindings_);
    computedStyle_ = other.computedStyle_;
    hasComputedStyle_ = other.hasComputedStyle_;
    value_ = std::move(other.value_);
    caret_ = other.caret_;
    selectionStart_ = other.selectionStart_;
    selectionEnd_ = other.selectionEnd_;
    checked_ = other.checked_;
    focusable_ = other.focusable_;
    focused_ = other.focused_;
    hovered_ = other.hovered_;
    pressed_ = other.pressed_;
    scrollable_ = other.scrollable_;
    scrollX_ = other.scrollX_;
    scrollY_ = other.scrollY_;

    setParentInternal(other.parent());
    setDocumentInternal(other.ownerDocument());
    setUidInternal(other.uid());

    other.setParentInternal(nullptr);
    other.setDocumentInternal(nullptr);
    other.setUidInternal(0);

    for (auto& child : children_) {
        child->setParentInternal(this);
    }
    for (auto& binding : bindings_) {
        if (binding) {
            binding->rebind(this);
        }
    }
}

Element& Element::operator=(Element&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    children_.clear();
    bindings_.clear();
    listeners_.clear();

    tag_ = std::move(other.tag_);
    id_ = std::move(other.id_);
    classes_ = std::move(other.classes_);
    attributes_ = std::move(other.attributes_);
    inlineStyles_ = std::move(other.inlineStyles_);
    children_ = std::move(other.children_);
    listeners_ = std::move(other.listeners_);
    bindings_ = std::move(other.bindings_);
    computedStyle_ = other.computedStyle_;
    hasComputedStyle_ = other.hasComputedStyle_;
    value_ = std::move(other.value_);
    caret_ = other.caret_;
    selectionStart_ = other.selectionStart_;
    selectionEnd_ = other.selectionEnd_;
    checked_ = other.checked_;
    focusable_ = other.focusable_;
    focused_ = other.focused_;
    hovered_ = other.hovered_;
    pressed_ = other.pressed_;
    scrollable_ = other.scrollable_;
    scrollX_ = other.scrollX_;
    scrollY_ = other.scrollY_;

    setParentInternal(other.parent());
    setDocumentInternal(other.ownerDocument());
    setUidInternal(other.uid());

    other.setParentInternal(nullptr);
    other.setDocumentInternal(nullptr);
    other.setUidInternal(0);

    for (auto& child : children_) {
        child->setParentInternal(this);
    }
    for (auto& binding : bindings_) {
        if (binding) {
            binding->rebind(this);
        }
    }
    return *this;
}

Element::~Element() = default;

Element&& Element::id(std::string value) {
    id_ = std::move(value);
    markStyleDirty();
    return std::move(*this);
}

Element&& Element::className(std::string values) {
    classes_ = splitClasses(values);
    markStyleDirty();
    return std::move(*this);
}

Element&& Element::addClass(std::string value) {
    if (value.empty()) {
        return std::move(*this);
    }
    if (std::find(classes_.begin(), classes_.end(), value) == classes_.end()) {
        classes_.push_back(std::move(value));
        markStyleDirty();
    }
    return std::move(*this);
}

Element&& Element::removeClass(std::string_view value) {
    const auto it = std::find(classes_.begin(), classes_.end(), value);
    if (it != classes_.end()) {
        classes_.erase(it);
        markStyleDirty();
    }
    return std::move(*this);
}

Element&& Element::toggleClass(std::string_view value, bool enabled) {
    if (enabled) {
        return addClass(std::string(value));
    }
    return removeClass(value);
}

bool Element::hasClass(std::string_view value) const {
    return std::find(classes_.begin(), classes_.end(), value) != classes_.end();
}

Element&& Element::attr(std::string name, std::string value) {
    if (name == "id") {
        return id(std::move(value));
    }
    if (name == "class" || name == "className") {
        return className(std::move(value));
    }
    if (name == "style") {
        style(std::move(value));
        return std::move(*this);
    }
    if (name == "value" && tag_ == "input") {
        value_ = value;
        caret_ = static_cast<int>(value_.size());
    }
    if (name == "checked" && tag_ == "input") {
        checked_ = isTruthy(value);
    }
    attributes_[std::move(name)] = std::move(value);
    markLayoutDirty();
    return std::move(*this);
}

bool Element::hasAttribute(std::string_view name) const {
    return attributes_.find(std::string(name)) != attributes_.end();
}

const std::string& Element::attribute(std::string_view name) const {
    static const std::string empty;
    const auto it = attributes_.find(std::string(name));
    return it == attributes_.end() ? empty : it->second;
}

void Element::removeAttribute(std::string_view name) {
    attributes_.erase(std::string(name));
    markLayoutDirty();
}

Element&& Element::text(std::string value) {
    clearChildren();
    auto node = std::make_unique<TextNode>(std::move(value));
    node->setParentInternal(this);
    if (ownerDocument() != nullptr) {
        node->attachTo(ownerDocument());
    }
    children_.push_back(std::move(node));
    markLayoutDirty();
    return std::move(*this);
}

std::string Element::textContent() const {
    std::string result;
    for (const auto& child : children_) {
        if (child->isText()) {
            result += static_cast<const TextNode&>(*child).text();
        } else if (child->isElement()) {
            result += static_cast<const Element&>(*child).textContent();
        }
    }
    return result;
}

Element&& Element::style(std::string property, std::string value) {
    inlineStyles_[std::move(property)] = std::move(value);
    markStyleDirty();
    return std::move(*this);
}

Element&& Element::style(std::string declarations) {
    std::size_t start = 0;
    while (start < declarations.size()) {
        std::size_t end = declarations.find(';', start);
        if (end == std::string::npos) {
            end = declarations.size();
        }
        const std::string_view declaration(declarations.data() + start, end - start);
        const std::size_t colon = declaration.find(':');
        if (colon != std::string_view::npos) {
            const std::string property = trim(declaration.substr(0, colon));
            const std::string value = trim(declaration.substr(colon + 1));
            if (!property.empty() && !value.empty()) {
                inlineStyles_[property] = value;
            }
        }
        start = end + 1;
    }
    markStyleDirty();
    return std::move(*this);
}

Element&& Element::child(Element&& childElement) {
    appendChild(std::make_unique<Element>(std::move(childElement)));
    return std::move(*this);
}

Element&& Element::child(TextNode&& textNodeValue) {
    appendChild(std::make_unique<TextNode>(std::move(textNodeValue)));
    return std::move(*this);
}

Element&& Element::child(std::unique_ptr<Node> childNode) {
    appendChild(std::move(childNode));
    return std::move(*this);
}

Element& Element::appendChild(std::unique_ptr<Node> childNode) {
    if (!childNode) {
        throw std::invalid_argument("Element::appendChild received a null child");
    }
    if (childNode.get() == this) {
        throw std::invalid_argument("An element cannot be its own child");
    }
    for (Node* ancestor = this; ancestor != nullptr; ancestor = ancestor->parent()) {
        if (ancestor == childNode.get()) {
            throw std::invalid_argument("Appending this child would create a cycle");
        }
    }
    if (childNode->parent() != nullptr) {
        throw std::invalid_argument("Child node already has a parent");
    }

    childNode->setParentInternal(this);
    if (auto* document = ownerDocument(); document != nullptr) {
        if (childNode->ownerDocument() != document) {
            childNode->attachTo(document);
        }
    } else {
        if (childNode->ownerDocument() != nullptr) {
            childNode->detachFrom(childNode->ownerDocument());
        }
    }
    children_.push_back(std::move(childNode));
    markLayoutDirty();
    return *this;
}

bool Element::removeChild(const Node& childNode) {
    const auto it = std::find_if(children_.begin(), children_.end(),
                                 [&childNode](const auto& child) {
                                     return child.get() == &childNode;
                                 });
    if (it == children_.end()) {
        return false;
    }
    if (auto* document = ownerDocument(); document != nullptr) {
        (*it)->detachFrom(document);
    }
    (*it)->setParentInternal(nullptr);
    children_.erase(it);
    markLayoutDirty();
    return true;
}

std::unique_ptr<Node> Element::detachChild(const Node& childNode) {
    const auto it = std::find_if(children_.begin(), children_.end(),
                                 [&childNode](const auto& child) {
                                     return child.get() == &childNode;
                                 });
    if (it == children_.end()) {
        return nullptr;
    }
    std::unique_ptr<Node> child = std::move(*it);
    children_.erase(it);
    if (auto* document = ownerDocument(); document != nullptr) {
        child->detachFrom(document);
    }
    child->setParentInternal(nullptr);
    markLayoutDirty();
    return child;
}

void Element::clearChildren() {
    if (children_.empty()) {
        return;
    }
    auto* document = ownerDocument();
    for (auto& child : children_) {
        if (document != nullptr) {
            child->detachFrom(document);
        }
        child->setParentInternal(nullptr);
    }
    children_.clear();
    markLayoutDirty();
}

Node* Element::childAt(std::size_t index) const {
    return index < children_.size() ? children_[index].get() : nullptr;
}

Element* Element::findById(std::string_view value) {
    if (id_ == value) {
        return this;
    }
    for (auto& child : children_) {
        if (child->isElement()) {
            if (auto* found = static_cast<Element*>(child.get())->findById(value)) {
                return found;
            }
        }
    }
    return nullptr;
}

const Element* Element::findById(std::string_view value) const {
    if (id_ == value) {
        return this;
    }
    for (const auto& child : children_) {
        if (child->isElement()) {
            if (const auto* found = static_cast<const Element*>(child.get())->findById(value)) {
                return found;
            }
        }
    }
    return nullptr;
}

std::vector<Element*> Element::findByClass(std::string_view value) {
    std::vector<Element*> result;
    if (hasClass(value)) {
        result.push_back(this);
    }
    for (auto& child : children_) {
        if (child->isElement()) {
            auto matches = static_cast<Element*>(child.get())->findByClass(value);
            result.insert(result.end(), matches.begin(), matches.end());
        }
    }
    return result;
}

std::vector<Element*> Element::findByTag(std::string_view tagName) {
    std::vector<Element*> result;
    if (tag_ == tagName) {
        result.push_back(this);
    }
    for (auto& child : children_) {
        if (child->isElement()) {
            auto matches = static_cast<Element*>(child.get())->findByTag(tagName);
            result.insert(result.end(), matches.begin(), matches.end());
        }
    }
    return result;
}

void Element::traverse(const std::function<void(Node&)>& visitor) {
    visitor(*this);
    for (auto& child : children_) {
        if (child->isElement()) {
            static_cast<Element&>(*child).traverse(visitor);
        } else {
            visitor(*child);
        }
    }
}

void Element::traverse(const std::function<void(const Node&)>& visitor) const {
    visitor(*this);
    for (const auto& child : children_) {
        if (child->isElement()) {
            static_cast<const Element&>(*child).traverse(visitor);
        } else {
            visitor(*child);
        }
    }
}

void Element::traverseElements(const std::function<void(Element&)>& visitor) {
    visitor(*this);
    for (auto& child : children_) {
        if (child->isElement()) {
            static_cast<Element&>(*child).traverseElements(visitor);
        }
    }
}

Element&& Element::on(EventType type, EventCallback callback) {
    listeners_[type].push_back(std::move(callback));
    return std::move(*this);
}

const std::vector<EventCallback>* Element::listeners(EventType type) const {
    const auto it = listeners_.find(type);
    return it == listeners_.end() ? nullptr : &it->second;
}

void Element::dispatchEvent(Event& event) {
    event.target = this;
    for (Node* node = this; node != nullptr; node = node->parent()) {
        if (!node->isElement()) {
            continue;
        }
        auto& element = static_cast<Element&>(*node);
        const auto it = element.listeners_.find(event.type);
        if (it == element.listeners_.end()) {
            if (event.propagationStopped) {
                break;
            }
            continue;
        }
        const auto callbacks = it->second;
        for (const auto& callback : callbacks) {
            if (event.propagationStopped) {
                break;
            }
            if (callback) {
                callback(event);
            }
        }
        if (event.propagationStopped) {
            break;
        }
    }
}

void Element::setValue(std::string value) {
    value_ = std::move(value);
    if (caret_ > static_cast<int>(value_.size())) {
        caret_ = static_cast<int>(value_.size());
    }
    if (selectionStart_ > static_cast<int>(value_.size())) {
        selectionStart_ = static_cast<int>(value_.size());
    }
    if (selectionEnd_ > static_cast<int>(value_.size())) {
        selectionEnd_ = static_cast<int>(value_.size());
    }
    markLayoutDirty();
}

void Element::setSelection(int start, int end) noexcept {
    const int size = static_cast<int>(value_.size());
    selectionStart_ = std::clamp(start, 0, size);
    selectionEnd_ = std::clamp(end, 0, size);
}

std::string Element::selectedText() const {
    if (!hasSelection()) {
        return {};
    }
    const int start = std::min(selectionStart_, selectionEnd_);
    const int end = std::max(selectionStart_, selectionEnd_);
    if (start < 0 || end > static_cast<int>(value_.size())) {
        return {};
    }
    return value_.substr(static_cast<std::size_t>(start),
                         static_cast<std::size_t>(end - start));
}

void Element::setChecked(bool checked) {
    if (checked_ == checked) {
        return;
    }
    checked_ = checked;
    markPaintDirty();
}

void Element::setScroll(float x, float y) {
    if (scrollX_ == x && scrollY_ == y) {
        return;
    }
    scrollX_ = x;
    scrollY_ = y;
    markLayoutDirty();
}

bool Element::isCheckbox() const noexcept {
    return (tag_ == "input" && attribute("type") == "checkbox") ||
           hasClass("eui-checkbox-box");
}

bool Element::isTextInput() const noexcept { return tag_ == "input" && !isCheckbox(); }

void Element::markDirty() { markStyleDirty(); }

void Element::markStyleDirty() {
    if (auto* document = ownerDocument()) {
        document->invalidateStyle();
    }
}

void Element::markLayoutDirty() {
    if (auto* document = ownerDocument()) {
        document->invalidateLayout();
    }
}

void Element::markPaintDirty() {
    if (auto* document = ownerDocument()) {
        document->invalidatePaint();
    }
}

std::unique_ptr<Node> Element::clone() const { return cloneElement(); }

std::unique_ptr<Element> Element::cloneElement() const {
    auto copy = std::make_unique<Element>(tag_);
    copy->id_ = id_;
    copy->classes_ = classes_;
    copy->attributes_ = attributes_;
    copy->inlineStyles_ = inlineStyles_;
    copy->computedStyle_ = computedStyle_;
    copy->hasComputedStyle_ = hasComputedStyle_;
    copy->value_ = value_;
    copy->caret_ = caret_;
    copy->selectionStart_ = selectionStart_;
    copy->selectionEnd_ = selectionEnd_;
    copy->checked_ = checked_;
    copy->focusable_ = focusable_;
    copy->scrollable_ = scrollable_;
    for (const auto& child : children_) {
        copy->appendChild(child->clone());
    }
    return copy;
}

void Element::setDocumentInternal(Document* document) noexcept {
    Node::setDocumentInternal(document);
}

void Element::setParentInternal(Node* parent) noexcept { Node::setParentInternal(parent); }

void Element::attachTo(Document* document) {
    Node::attachTo(document);
    for (auto& child : children_) {
        child->attachTo(document);
    }
}

void Element::detachFrom(Document* document) {
    Node::detachFrom(document);
    for (auto& child : children_) {
        child->detachFrom(document);
    }
}

void Element::assignOwnership() {
    for (auto& child : children_) {
        child->setParentInternal(this);
        if (auto* document = ownerDocument(); document != nullptr) {
            child->attachTo(document);
        }
    }
}

void Element::reindex() { assignOwnership(); }

bool Element::hasInlineStyle(std::string_view property) const {
    return inlineStyles_.find(std::string(property)) != inlineStyles_.end();
}

Element div() { return Element("div"); }
Element h1() { return Element("h1"); }
Element h2() { return Element("h2"); }
Element p() { return Element("p"); }
Element span() { return Element("span"); }

Element button(std::string textValue) {
    Element element("button");
    if (!textValue.empty()) {
        element.text(std::move(textValue));
    }
    return element;
}

Element input(std::string placeholder) {
    Element element("input");
    element.attr("type", "text");
    if (!placeholder.empty()) {
        element.attr("placeholder", std::move(placeholder));
    }
    return element;
}

Element label(std::string textValue) {
    Element element("label");
    if (!textValue.empty()) {
        element.text(std::move(textValue));
    }
    return element;
}

Element image(std::string source) {
    Element element("image");
    if (!source.empty()) {
        element.attr("src", std::move(source));
    }
    return element;
}

Element list() { return Element("list"); }

TextNode textNode(std::string textValue) { return TextNode(std::move(textValue)); }

} // namespace EntroUI
