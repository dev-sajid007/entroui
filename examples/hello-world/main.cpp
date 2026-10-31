#include <entroui/entroui.hpp>

#include <iostream>

using namespace EntroUI;

int main() {
    App app;
    app.title("Hello EntroUI").size(900, 600).background("#f8fafc");

    auto page = div()
                    .className("container")
                    .style("padding", "40px")
                    .child(h1().text("Hello EntroUI"))
                    .child(p().text("Welcome to your first native C++ desktop application."))
                    .child(button("Click Me").onClick([](Event&) {
                        std::cout << "Button clicked!\n";
                    }));

    app.mount(std::move(page));
    return app.run();
}
