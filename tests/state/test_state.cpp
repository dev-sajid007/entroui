#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "entroui/entroui.hpp"

using namespace EntroUI;

TEST(State, GetSetAndSubscribe) {
    auto counter = state<int>(0);
    int observed = -1;
    int calls = 0;
    auto subscription = counter.subscribe([&](const int& value) {
        observed = value;
        ++calls;
    });
    EXPECT_EQ(counter.get(), 0);
    counter.set(5);
    EXPECT_EQ(counter.get(), 5);
    EXPECT_EQ(observed, 5);
    EXPECT_EQ(calls, 1);

    subscription.cancel();
    counter.set(8);
    EXPECT_EQ(observed, 5);
    EXPECT_EQ(calls, 1);
}

TEST(State, UpdateHelper) {
    auto counter = state<int>(2);
    counter.update([](int value) { return value * 3; });
    EXPECT_EQ(counter.get(), 6);
}

TEST(State, BindTextUpdatesElement) {
    auto counter = state<int>(0);
    auto heading = h1().bindText(counter);
    Document document;
    document.setRoot(std::make_unique<Element>(std::move(heading)));
    EXPECT_EQ(document.root().textContent(), "0");
    counter.set(42);
    EXPECT_EQ(document.root().textContent(), "42");
}

TEST(State, BindStringText) {
    auto name = state<std::string>("Sajid");
    auto paragraph = p().bindText(name);
    Document document;
    document.setRoot(std::make_unique<Element>(std::move(paragraph)));
    EXPECT_EQ(document.root().textContent(), "Sajid");
    name.set("EntroUI");
    EXPECT_EQ(document.root().textContent(), "EntroUI");
}

TEST(State, BindingStopsWhenElementIsDestroyed) {
    auto value = state<int>(1);
    {
        auto element = div().bindText(value);
        Document document;
        document.setRoot(std::make_unique<Element>(std::move(element)));
        value.set(2);
        EXPECT_EQ(document.root().textContent(), "2");
    }
    value.set(3);
    SUCCEED();
}

TEST(State, DirtyFlagsAreUpdatedByBindings) {
    auto value = state<int>(1);
    auto element = div().bindText(value);
    Document document;
    document.setRoot(std::make_unique<Element>(std::move(element)));
    document.clearDirty();
    value.set(9);
    EXPECT_FALSE(document.styleDirty());
    EXPECT_TRUE(document.layoutDirty());
    EXPECT_TRUE(document.paintDirty());
}

class CounterComponent : public Component {
public:
    CounterComponent() : Component("Counter") {}

    Element render() override {
        ++renders;
        count.set(count.get() + 1);
        return div().child(h1().bindText(count));
    }

    State<int> count{0};
    int renders = 0;
};

TEST(State, ComponentRendersAndTracksUpdates) {
    CounterComponent component;
    bool updateCalled = false;
    component.setUpdateCallback([&] { updateCalled = true; });
    Element root = component.takeRoot();
    EXPECT_EQ(component.renders, 1);
    EXPECT_FALSE(root.children().empty());

    component.requestUpdate();
    EXPECT_TRUE(component.updateRequested());
    EXPECT_TRUE(updateCalled);
    component.clearUpdateRequest();
    EXPECT_FALSE(component.updateRequested());

    Element rerendered = component.takeRoot();
    EXPECT_EQ(component.renders, 2);
    EXPECT_FALSE(rerendered.children().empty());
}
