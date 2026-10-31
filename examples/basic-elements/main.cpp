#include <entroui/entroui.hpp>

#include <filesystem>
#include <iostream>

using namespace EntroUI;

int main() {
    const std::filesystem::path directory = ENTROUI_EXAMPLE_DIR;

    App app;
    app.background("#f8fafc")
        .load(directory / "ui" / "app.eui")
        .stylesheet(directory / "ui" / "styles.css");

    app.document().findById("primary")->onClick([](Event&) {
        std::cout << "Primary button clicked\n";
    });
    app.document().findById("secondary")->onClick([](Event&) {
        std::cout << "Secondary button clicked\n";
    });

    return app.run();
}
