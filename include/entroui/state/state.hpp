#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace EntroUI {

class Element;

namespace detail {

template <typename T>
std::string toDisplayString(const T& value) {
    if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else if constexpr (std::is_same_v<T, std::string_view>) {
        return std::string(value);
    } else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
        return std::string(value);
    } else if constexpr (std::is_same_v<T, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }
}

} // namespace detail

class Subscription {
public:
    Subscription() = default;
    ~Subscription() { cancel(); }

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    Subscription(Subscription&& other) noexcept : control_(std::move(other.control_)) {}

    Subscription& operator=(Subscription&& other) noexcept {
        if (this != &other) {
            cancel();
            control_ = std::move(other.control_);
        }
        return *this;
    }

    void cancel() {
        if (control_) {
            if (control_->cancel) {
                control_->cancel();
            }
            control_.reset();
        }
    }

    [[nodiscard]] bool active() const noexcept { return static_cast<bool>(control_); }

private:
    struct Control {
        std::function<void()> cancel;
    };

    std::shared_ptr<Control> control_;

    template <typename>
    friend class State;
};

template <typename T>
class State {
public:
    struct Core {
        explicit Core(T initial) : value(std::move(initial)) {}

        T value;
        std::uint64_t nextId = 1;
        std::vector<std::pair<std::uint64_t, std::function<void(const T&)>>> subscribers;
    };

    State() : core_(std::make_shared<Core>(T{})) {}
    explicit State(T initial) : core_(std::make_shared<Core>(std::move(initial))) {}

    [[nodiscard]] const T& get() const noexcept { return core_->value; }

    void set(T value) {
        core_->value = std::move(value);
        notify();
    }

    template <typename F>
    void update(F&& updater) {
        set(std::invoke(std::forward<F>(updater), core_->value));
    }

    template <typename F>
    Subscription subscribe(F&& callback) {
        const std::uint64_t id = core_->nextId++;
        core_->subscribers.emplace_back(
            id, std::function<void(const T&)>(std::forward<F>(callback)));

        auto control = std::make_shared<Subscription::Control>();
        std::weak_ptr<Core> weakCore = core_;
        control->cancel = [weakCore, id] {
            if (auto core = weakCore.lock()) {
                auto& subscribers = core->subscribers;
                subscribers.erase(
                    std::remove_if(subscribers.begin(), subscribers.end(),
                                   [id](const auto& entry) { return entry.first == id; }),
                    subscribers.end());
            }
        };
        Subscription subscription;
        subscription.control_ = std::move(control);
        return subscription;
    }

    void notify() {
        auto subscribers = core_->subscribers;
        for (auto& [id, callback] : subscribers) {
            if (callback) {
                callback(core_->value);
            }
        }
    }

    [[nodiscard]] std::shared_ptr<Core> core() const noexcept { return core_; }

private:
    std::shared_ptr<Core> core_;
};

template <typename T>
State<T> state(T initial) {
    return State<T>(std::move(initial));
}

class Binding {
public:
    virtual ~Binding() = default;
    virtual void refresh() = 0;
    virtual void rebind(Element* target) = 0;
};

} // namespace EntroUI
