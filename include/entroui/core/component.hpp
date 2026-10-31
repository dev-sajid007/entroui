#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "entroui/core/element.hpp"

namespace EntroUI {

class Component {
public:
    explicit Component(std::string name = {});
    virtual ~Component();

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    [[nodiscard]] virtual Element render() = 0;
    virtual void onMount() {}
    virtual void onUnmount() {}

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    void requestUpdate();
    void setUpdateCallback(std::function<void()> callback);

    [[nodiscard]] bool updateRequested() const noexcept { return updateRequested_; }
    void clearUpdateRequest() noexcept { updateRequested_ = false; }

    [[nodiscard]] Element takeRoot();
    [[nodiscard]] Element* root() const noexcept { return root_.get(); }
    void setRoot(std::unique_ptr<Element> root);

private:
    std::string name_;
    std::unique_ptr<Element> root_;
    std::function<void()> updateCallback_;
    bool updateRequested_ = false;
};

} // namespace EntroUI
