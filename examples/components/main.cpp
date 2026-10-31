#include <entroui/entroui.hpp>

#include <iostream>
#include <memory>

using namespace EntroUI;

class ProfileCard : public Component {
public:
    ProfileCard() : Component("ProfileCard") {}

    Element render() override {
        return div()
            .className("profile")
            .style("background", "#ffffff")
            .style("border", "1px solid #e2e8f0")
            .style("border-radius", "14px")
            .style("padding", "28px")
            .style("width", "420px")
            .child(h1().bindText(clicks))
            .child(p().text("Component-local state and lifecycle hooks."))
            .child(button("Re-render component").onClick([this](Event&) {
                clicks.set(clicks.get() + 1);
                requestUpdate();
            }))
            .child(button("+1 without re-render")
                       .className("ghost")
                       .onClick([this](Event&) { clicks.set(clicks.get() + 1); }));
    }

    void onMount() override { std::cout << "ProfileCard mounted\n"; }
    void onUnmount() override { std::cout << "ProfileCard unmounted\n"; }

private:
    State<int> clicks{0};
};

int main() {
    App app;
    app.title("EntroUI Components").size(900, 600).background("#f1f5f9");

    app.mountComponent(std::make_unique<ProfileCard>());
    return app.run();
}
