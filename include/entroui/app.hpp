#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "entroui/config.hpp"
#include "entroui/style/style.hpp"

namespace EntroUI {

class Document;
class Element;
class Component;

class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    App& title(std::string title);
    App& size(int width, int height);
    App& background(Color color);
    App& background(std::string_view cssColor);

    App& mount(Element&& root);
    App& mountComponent(std::unique_ptr<Component> component);
    App& load(const std::filesystem::path& markupPath);
    App& stylesheet(const std::filesystem::path& cssPath);
    App& stylesheetText(std::string css, std::string sourceName = "<stylesheet>");
    App& font(const std::filesystem::path& fontPath);
    App& resourceDirectory(const std::filesystem::path& directory);

    [[nodiscard]] Document& document();
    [[nodiscard]] bool hasDocument() const noexcept;
    [[nodiscard]] const std::string& windowTitle() const noexcept;
    [[nodiscard]] int windowWidth() const noexcept;
    [[nodiscard]] int windowHeight() const noexcept;

    int run();
    void quit();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace EntroUI
