#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

#include "entroui/entroui.hpp"

using namespace EntroUI;

namespace {

class TempFile {
public:
    TempFile(std::string name, std::string contents) {
        path_ = std::filesystem::temp_directory_path() / std::move(name);
        std::ofstream stream(path_, std::ios::binary);
        stream << contents;
    }
    ~TempFile() {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

bool hasTextCommand(const RenderCommandList& commands, const std::string& text) {
    for (const auto& command : commands.commands()) {
        if (const auto* textCommand = std::get_if<TextCommand>(&command)) {
            if (textCommand->text == text) {
                return true;
            }
        }
    }
    return false;
}

bool hasRectColor(const RenderCommandList& commands, const Color& color) {
    for (const auto& command : commands.commands()) {
        if (const auto* rect = std::get_if<RectCommand>(&command)) {
            if (rect->color == color) {
                return true;
            }
        }
    }
    return false;
}

int countCommandsOfType(const RenderCommandList& commands, bool pushClip) {
    int count = 0;
    for (const auto& command : commands.commands()) {
        if (pushClip && std::holds_alternative<PushClipCommand>(command)) {
            ++count;
        }
        if (!pushClip && std::holds_alternative<PopClipCommand>(command)) {
            ++count;
        }
    }
    return count;
}

} // namespace

TEST(Integration, MarkupToRenderCommands) {
    Parser parser;
    auto parsed = parser.parse(R"(
        <div class="container">
          <h1>Hello EntroUI</h1>
          <p>Welcome to my application.</p>
          <button id="submit">Click Me</button>
        </div>
    )");
    ASSERT_TRUE(parsed.ok());
    auto built = buildDocument(*parsed.value);
    ASSERT_TRUE(built.ok());

    auto stylesheet = StyleSheet::parse(R"(
        .container { width: 500px; height: 300px; background: #ffffff; padding: 20px; }
        h1 { color: #111827; }
        button { background: #2563eb; }
    )");
    ASSERT_TRUE(stylesheet.ok());

    auto& document = *built.document;
    document.setViewport(640.0F, 480.0F);
    computeDocumentStyles(document, defaultThemeStylesheet().get(), &*stylesheet.value);
    LayoutTree layout;
    FixedTextMeasurer measurer;
    layout.layout(document, 640.0F, 480.0F, measurer);
    ASSERT_TRUE(layout.valid());

    const auto commands = buildRenderCommands(layout);
    EXPECT_FALSE(commands.empty());
    EXPECT_TRUE(hasTextCommand(commands, "Hello EntroUI"));
    EXPECT_TRUE(hasTextCommand(commands, "Welcome to my application."));
    EXPECT_TRUE(hasTextCommand(commands, "Click Me"));
    EXPECT_TRUE(hasRectColor(commands, Color(37, 99, 235, 255)));
}

TEST(Integration, ScrollContainersEmitClipCommands) {
    auto root = div();
    root.child(Widgets::scrollView(div().child(div().style("height", "400px")), 120.0F));
    Document document;
    document.setRoot(std::make_unique<Element>(std::move(root)));
    document.setViewport(400.0F, 300.0F);
    computeDocumentStyles(document, defaultThemeStylesheet().get(), nullptr);
    LayoutTree layout;
    FixedTextMeasurer measurer;
    layout.layout(document, 400.0F, 300.0F, measurer);
    const auto commands = buildRenderCommands(layout);
    EXPECT_EQ(countCommandsOfType(commands, true), 1);
    EXPECT_EQ(countCommandsOfType(commands, false), 1);
}

TEST(Integration, AppMountsCppTree) {
    App app;
    app.title("My App").size(800, 600);
    app.mount(div().id("root").child(h1().text("Hello")));
    EXPECT_TRUE(app.hasDocument());
    EXPECT_EQ(app.document().root().id(), "root");
    EXPECT_EQ(app.windowTitle(), "My App");
    EXPECT_EQ(app.windowWidth(), 800);
    EXPECT_EQ(app.windowHeight(), 600);
}

TEST(Integration, AppLoadsMarkupAndWindowMetadata) {
    TempFile markup("entroui_test_app.eui", R"(
        <app>
          <window title="Markup App" width="720" height="480">
            <div class="container"><h1>Loaded</h1></div>
          </window>
        </app>
    )");
    App app;
    app.load(markup.path());
    EXPECT_TRUE(app.hasDocument());
    EXPECT_EQ(app.windowTitle(), "Markup App");
    EXPECT_EQ(app.windowWidth(), 720);
    EXPECT_EQ(app.windowHeight(), 480);
    EXPECT_FALSE(app.document().findByTag("h1").empty());
}

TEST(Integration, AppLoadsExternalStylesheet) {
    TempFile css("entroui_test_style.css", ".container { background: #123456; }");
    App app;
    app.mount(div().className("container"));
    app.stylesheet(css.path());
    auto& document = app.document();
    document.setViewport(400.0F, 300.0F);
    computeDocumentStyles(document, defaultThemeStylesheet().get(), document.stylesheet());
    EXPECT_EQ(document.root().computedStyle().background, Color(18, 52, 86, 255));
}

TEST(Integration, AppRejectsInvalidMarkup) {
    TempFile markup("entroui_test_bad.eui", "<div><span></div>");
    App app;
    EXPECT_THROW(app.load(markup.path()), std::runtime_error);
}

TEST(Integration, AppRejectsInvalidStylesheet) {
    App app;
    EXPECT_THROW(app.stylesheetText(".x { width: 10px;", "<bad>"), std::runtime_error);
}

TEST(Integration, TextSelectionEmitsHighlight) {
    auto root = div();
    auto field = Widgets::textInput("Type", "hello");
    field.id("input");
    field.setSelection(1, 3);
    field.setFocused(true);
    root.child(std::move(field));
    Document document;
    document.setRoot(std::make_unique<Element>(std::move(root)));
    document.setViewport(400.0F, 200.0F);
    computeDocumentStyles(document, defaultThemeStylesheet().get(), nullptr);
    LayoutTree layout;
    FixedTextMeasurer measurer;
    layout.layout(document, 400.0F, 200.0F, measurer);
    const auto commands = buildRenderCommands(layout);
    EXPECT_TRUE(hasRectColor(commands, Color(191, 219, 254, 255)));
}
