#include <entroui/entroui.hpp>

#include <iostream>

using namespace EntroUI;

int main() {
    App app;
    app.title("EntroUI Buttons").size(800, 600).background("#f8fafc");

    auto count = state<int>(0);

    auto page = div()
                    .style("padding", "40px")
                    .child(h1().text("Buttons and events"))
                    .child(p().text("Click the buttons to update reactive state."))
                    .child(h1().className("count").bindText(count))
                    .child(div()
                               .style("display", "flex")
                               .style("gap", "12px")
                               .style("margin-top", "20px")
                               .child(button("Increment").onClick([&](Event&) {
                                   count.set(count.get() + 1);
                               }))
                               .child(button("Decrement").onClick([&](Event&) {
                                   count.set(count.get() - 1);
                               }))
                               .child(button("Reset").onClick([&](Event&) {
                                   count.set(0);
                               })))
                    .child(p().style("margin-top", "24px")
                               .text("Buttons receive click, hover, press and focus events."));

    app.mount(std::move(page));
    return app.run();
}
