#include <entroui/entroui.hpp>

#include <iostream>

using namespace EntroUI;

int main() {
    App app;
    app.title("EntroUI Forms").size(800, 700).background("#f8fafc");

    auto name = state<std::string>("");
    auto output = state<std::string>("No message yet");

    auto nameInput = Widgets::textInput("Your name");
    nameInput.id("name");
    nameInput.onChange([&](Event& event) {
        name.set(static_cast<Element&>(*event.target).value());
    });

    auto remember = Widgets::checkbox("Remember me", true);

    auto page = div()
                    .style("padding", "40px")
                    .child(h1().text("Form controls"))
                    .child(label("Name"))
                    .child(std::move(nameInput))
                    .child(div().style("margin-top", "16px").child(std::move(remember)))
                    .child(div()
                               .style("margin-top", "24px")
                               .child(button("Say hello").onClick([&](Event&) {
                                   if (name.get().empty()) {
                                       output.set("Please enter a name");
                                   } else {
                                       output.set("Hello, " + name.get() + "!");
                                   }
                               })))
                    .child(p().style("margin-top", "24px").bindText(output));

    app.mount(std::move(page));
    return app.run();
}
