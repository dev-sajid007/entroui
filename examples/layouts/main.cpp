#include <entroui/entroui.hpp>

#include <iostream>

using namespace EntroUI;

namespace {

Element panel(std::string title, std::string body, std::string accent) {
    return div()
        .className("panel")
        .style("flex", "1")
        .style("background", "#ffffff")
        .style("border", "1px solid #e2e8f0")
        .style("border-radius", "12px")
        .style("padding", "20px")
        .child(h2().text(std::move(title)))
        .child(p().text(std::move(body)))
        .child(div()
                   .style("height", "6px")
                   .style("background", accent)
                   .style("border-radius", "3px")
                   .style("margin-top", "16px"));
}

} // namespace

int main() {
    App app;
    app.title("EntroUI Layouts").size(1000, 700).background("#f1f5f9");

    auto side = div()
                    .className("sidebar")
                    .style("width", "220px")
                    .style("display", "flex")
                    .style("flex-direction", "column")
                    .style("gap", "8px")
                    .style("background", "#ffffff")
                    .style("border-radius", "12px")
                    .style("padding", "18px")
                    .child(h2().text("Navigation"))
                    .child(button("Dashboard").onClick([](Event&) {
                        std::cout << "Dashboard\n";
                    }))
                    .child(button("Projects").className("ghost"))
                    .child(button("Settings").className("ghost"));

    auto cards = div()
                     .className("cards")
                     .style("display", "flex")
                     .style("flex-direction", "row")
                     .style("gap", "16px")
                     .child(panel("Flexbox", "Row and column layout with gap, alignment and "
                                             "justification.",
                                  "#2563eb"))
                     .child(panel("Box model", "Padding, margins, borders, min/max sizes and "
                                               "percentages.",
                                  "#16a34a"))
                     .child(panel("Text", "Wrapping, alignment and line height built into the "
                                          "layout engine.",
                                  "#f59e0b"));

    auto content = div()
                       .className("content")
                       .style("width", "700px")
                       .child(h1().text("Layout Engine"))
                       .child(p().text("Build structured desktop layouts with a CSS-inspired "
                                       "flex layout engine."))
                       .child(std::move(cards));

    auto page = div()
                    .className("page")
                    .style("display", "flex")
                    .style("flex-direction", "row")
                    .style("gap", "24px")
                    .style("padding", "24px")
                    .child(std::move(side))
                    .child(std::move(content));

    app.mount(std::move(page));
    return app.run();
}
