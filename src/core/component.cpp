#include "entroui/core/component.hpp"

#include <stdexcept>

namespace EntroUI {

Component::Component(std::string name) : name_(std::move(name)) {}

Component::~Component() = default;

void Component::requestUpdate() {
    updateRequested_ = true;
    if (updateCallback_) {
        updateCallback_();
    }
}

void Component::setUpdateCallback(std::function<void()> callback) {
    updateCallback_ = std::move(callback);
}

Element Component::takeRoot() {
    if (!root_) {
        root_ = std::make_unique<Element>(render());
    }
    auto result = std::move(*root_);
    root_.reset();
    return result;
}

void Component::setRoot(std::unique_ptr<Element> root) { root_ = std::move(root); }

} // namespace EntroUI
