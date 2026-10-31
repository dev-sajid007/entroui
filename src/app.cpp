#include "entroui/app.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>

#include "entroui/core/component.hpp"
#include "entroui/core/document.hpp"
#include "entroui/core/element.hpp"
#include "entroui/events/event_dispatcher.hpp"
#include "entroui/layout/layout_engine.hpp"
#include "entroui/parser/builder.hpp"
#include "entroui/parser/parser.hpp"
#include "entroui/renderer/render_tree.hpp"
#include "entroui/resource/resource_manager.hpp"
#include "entroui/style/computed_style.hpp"
#include "entroui/style/stylesheet.hpp"

#if ENTROUI_HAS_SDL
#include <SDL3/SDL.h>

#include "platform/sdl/sdl_platform.hpp"
#include "renderer/sdl/sdl_font.hpp"
#include "renderer/sdl/sdl_renderer.hpp"
#include "renderer/sdl/sdl_text_measurer.hpp"
#endif

namespace EntroUI {

namespace {

bool hasErrors(const std::vector<Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }
    return false;
}

std::string formatDiagnostics(const std::vector<Diagnostic>& diagnostics) {
    std::string message;
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity != DiagnosticSeverity::Error) {
            continue;
        }
        if (!message.empty()) {
            message += '\n';
        }
        message += diagnostic.format();
    }
    return message;
}

void logWarnings(const std::vector<Diagnostic>& diagnostics, spdlog::level::level_enum level,
                 const char* context) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::Warning) {
            spdlog::log(level, "{}: {}", context, diagnostic.format());
        }
    }
}

#if ENTROUI_HAS_SDL
int translateKey(SDL_Keycode key) {
    switch (key) {
    case SDLK_BACKSPACE:
        return Keys::Backspace;
    case SDLK_TAB:
        return Keys::Tab;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_RETURN2:
        return Keys::Enter;
    case SDLK_ESCAPE:
        return Keys::Escape;
    case SDLK_SPACE:
        return Keys::Space;
    case SDLK_DELETE:
        return Keys::Delete;
    case SDLK_LEFT:
        return Keys::Left;
    case SDLK_RIGHT:
        return Keys::Right;
    case SDLK_UP:
        return Keys::Up;
    case SDLK_DOWN:
        return Keys::Down;
    case SDLK_HOME:
        return Keys::Home;
    case SDLK_END:
        return Keys::End;
    default:
        break;
    }
    const auto code = static_cast<int>(key);
    return code > 0 && code < 128 ? code : 0;
}

std::string boldVariant(const std::string& path) {
    if (path.empty()) {
        return {};
    }
    std::string result = path;
    const std::string regular = "Regular";
    const auto position = result.find(regular);
    if (position == std::string::npos) {
        return {};
    }
    result.replace(position, regular.size(), "Bold");
    std::error_code error;
    if (std::filesystem::exists(result, error) && !error) {
        return result;
    }
    return {};
}
#endif

} // namespace

struct App::Impl {
    std::string title = "EntroUI Application";
    int width = 900;
    int height = 600;
    bool titleSet = false;
    bool sizeSet = false;
    Color background{245, 247, 250, 255};
    std::unique_ptr<Document> document;
    std::shared_ptr<const StyleSheet> userStylesheet;
    std::filesystem::path fontPath;
    std::filesystem::path resourceDirectory;
    std::filesystem::path markupPath;
    bool resourceDirectorySet = false;
    bool quitRequested = false;
    std::unique_ptr<Component> component;
    bool componentDirty = false;

    Document& ensureDocument() {
        if (!document) {
            document = std::make_unique<Document>();
            if (userStylesheet) {
                document->setStylesheet(userStylesheet);
            }
        }
        return *document;
    }

    void refreshComponent() {
        if (!component || !document) {
            return;
        }
        component->onUnmount();
        auto root = component->takeRoot();
        document->setRoot(std::make_unique<Element>(std::move(root)));
        component->onMount();
        component->clearUpdateRequest();
        componentDirty = false;
        document->invalidateStyle();
    }

    int run();
};

int App::Impl::run() {
#if !ENTROUI_HAS_SDL
    throw std::runtime_error(
        "EntroUI was built without the SDL backend; rebuild with ENTROUI_BUILD_SDL=ON");
#else
    if (!document || !document->hasRoot()) {
        throw std::runtime_error("App::run called without a mounted UI tree");
    }
    if (resourceDirectory.empty()) {
        resourceDirectory = std::filesystem::current_path();
    }

    detail::SDLPlatform platform;
    platform.createWindow(title, width, height);
    platform.createRenderer();
    platform.startTextInput();

    int windowWidth = width;
    int windowHeight = height;
    platform.windowSize(windowWidth, windowHeight);
    document->setViewport(static_cast<float>(windowWidth), static_cast<float>(windowHeight));

    {
        detail::FontCache fonts;
        if (!fontPath.empty()) {
            fonts.setDefaultFont(fontPath.string(), boldVariant(fontPath.string()));
        }
        detail::SDLTextMeasurer measurer(fonts);
        ResourceManager resources(resourceDirectory);
        detail::SDLRenderer renderer(platform.renderer(), fonts, resources);
        LayoutTree layout;
        EventDispatcher dispatcher(*document, layout);
        const auto theme = defaultThemeStylesheet();

        auto lastFrame = std::chrono::steady_clock::now();
        while (!quitRequested) {
            bool quit = false;
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    quit = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    int newWidth = 0;
                    int newHeight = 0;
                    platform.windowSize(newWidth, newHeight);
                    if (newWidth > 0 && newHeight > 0) {
                        document->setViewport(static_cast<float>(newWidth),
                                              static_cast<float>(newHeight));
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_EXPOSED:
                    document->invalidatePaint();
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    dispatcher.handleMouseMove(event.motion.x, event.motion.y);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    dispatcher.handleMouseDown(event.button.x, event.button.y,
                                               event.button.button);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    dispatcher.handleMouseUp(event.button.x, event.button.y,
                                             event.button.button);
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    dispatcher.handleWheel(event.wheel.mouse_x, event.wheel.mouse_y,
                                           event.wheel.x * 40.0F, -event.wheel.y * 40.0F);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    dispatcher.handleKeyDown(
                        translateKey(event.key.key), static_cast<int>(event.key.scancode),
                        (event.key.mod & SDL_KMOD_SHIFT) != 0 ? 1 : 0, event.key.repeat);
                    break;
                case SDL_EVENT_KEY_UP:
                    dispatcher.handleKeyUp(
                        translateKey(event.key.key), static_cast<int>(event.key.scancode),
                        (event.key.mod & SDL_KMOD_SHIFT) != 0 ? 1 : 0);
                    break;
                case SDL_EVENT_TEXT_INPUT:
                    dispatcher.handleTextInput(event.text.text);
                    break;
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    dispatcher.setFocus(nullptr);
                    break;
                default:
                    break;
                }
            }
            if (quit) {
                break;
            }
            if (componentDirty) {
                dispatcher.clearInteraction();
                refreshComponent();
            }

            if (document->styleDirty()) {
                computeDocumentStyles(*document, theme.get(), document->stylesheet());
            }
            if (document->layoutDirty()) {
                int currentWidth = 0;
                int currentHeight = 0;
                platform.windowSize(currentWidth, currentHeight);
                if (currentWidth <= 0 || currentHeight <= 0) {
                    currentWidth = width;
                    currentHeight = height;
                }
                document->setViewport(static_cast<float>(currentWidth),
                                      static_cast<float>(currentHeight));
                layout.layout(*document, static_cast<float>(currentWidth),
                              static_cast<float>(currentHeight), measurer, &resources);
                document->clearLayoutDirty();
            }
            if (document->paintDirty()) {
                const auto commands = buildRenderCommands(layout, dispatcher.focused());
                int currentWidth = 0;
                int currentHeight = 0;
                platform.windowSize(currentWidth, currentHeight);
                renderer.beginFrame(currentWidth, currentHeight, background);
                executeRenderCommands(renderer, commands);
                renderer.endFrame();
                document->clearPaintDirty();
            }

            const auto now = std::chrono::steady_clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrame);
            constexpr auto frameBudget = std::chrono::microseconds(16667);
            if (elapsed < frameBudget) {
                std::this_thread::sleep_for(frameBudget - elapsed);
            }
            lastFrame = std::chrono::steady_clock::now();
        }

        dispatcher.clearInteraction();
        document->clearDirty();
    }

    if (component) {
        component->onUnmount();
    }
    platform.stopTextInput();
    return 0;
#endif
}

App::App() : impl_(std::make_unique<Impl>()) {}

App::~App() = default;

App& App::title(std::string title) {
    impl_->title = std::move(title);
    impl_->titleSet = true;
    return *this;
}

App& App::size(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("App size must be positive");
    }
    impl_->width = width;
    impl_->height = height;
    impl_->sizeSet = true;
    return *this;
}

App& App::background(Color color) {
    impl_->background = color;
    return *this;
}

App& App::background(std::string_view cssColor) {
    impl_->background = Color::parse(cssColor);
    return *this;
}

App& App::mount(Element&& root) {
    auto& document = impl_->ensureDocument();
    document.setRoot(std::make_unique<Element>(std::move(root)));
    impl_->component.reset();
    impl_->componentDirty = false;
    document.invalidateStyle();
    return *this;
}

App& App::mountComponent(std::unique_ptr<Component> component) {
    if (!component) {
        throw std::invalid_argument("App::mountComponent received a null component");
    }
    auto& document = impl_->ensureDocument();
    if (document.hasRoot()) {
        throw std::logic_error("Cannot mount a component when a root tree is already mounted");
    }
    impl_->component = std::move(component);
    impl_->component->setUpdateCallback([this] {
        impl_->componentDirty = true;
        if (impl_->document) {
            impl_->document->invalidateStyle();
        }
    });
    impl_->component->onMount();
    auto root = impl_->component->takeRoot();
    document.setRoot(std::make_unique<Element>(std::move(root)));
    document.invalidateStyle();
    return *this;
}

App& App::load(const std::filesystem::path& markupPath) {
    std::ifstream stream(markupPath, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open markup file: " + markupPath.string());
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();

    Parser parser;
    auto parsed = parser.parse(buffer.str(), markupPath.string());
    if (hasErrors(parsed.diagnostics)) {
        throw std::runtime_error(formatDiagnostics(parsed.diagnostics));
    }
    logWarnings(parsed.diagnostics, spdlog::level::warn, "Markup");
    if (!parsed.value) {
        throw std::runtime_error("Failed to parse markup: " + markupPath.string());
    }

    auto built = buildDocument(*parsed.value);
    if (hasErrors(built.diagnostics) || !built.document) {
        throw std::runtime_error(formatDiagnostics(built.diagnostics));
    }
    logWarnings(built.diagnostics, spdlog::level::warn, "Markup");

    impl_->document = std::move(built.document);
    if (impl_->userStylesheet) {
        impl_->document->setStylesheet(impl_->userStylesheet);
    }
    if (built.title && !impl_->titleSet) {
        impl_->title = *built.title;
    }
    if (built.width && !impl_->sizeSet) {
        impl_->width = *built.width;
    }
    if (built.height && !impl_->sizeSet) {
        impl_->height = *built.height;
    }
    impl_->markupPath = markupPath;
    impl_->component.reset();
    impl_->componentDirty = false;
    if (!impl_->resourceDirectorySet) {
        impl_->resourceDirectory = markupPath.has_parent_path()
                                       ? markupPath.parent_path()
                                       : std::filesystem::current_path();
    }
    impl_->document->invalidateStyle();
    return *this;
}

App& App::stylesheet(const std::filesystem::path& cssPath) {
    std::ifstream stream(cssPath, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open stylesheet: " + cssPath.string());
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return stylesheetText(buffer.str(), cssPath.string());
}

App& App::stylesheetText(std::string css, std::string sourceName) {
    auto parsed = StyleSheet::parse(css, std::move(sourceName));
    if (hasErrors(parsed.diagnostics)) {
        throw std::runtime_error(formatDiagnostics(parsed.diagnostics));
    }
    logWarnings(parsed.diagnostics, spdlog::level::warn, "Stylesheet");
    if (!parsed.value) {
        throw std::runtime_error("Failed to parse stylesheet");
    }
    impl_->userStylesheet = std::make_shared<const StyleSheet>(std::move(*parsed.value));
    if (impl_->document) {
        impl_->document->setStylesheet(impl_->userStylesheet);
    }
    return *this;
}

App& App::font(const std::filesystem::path& fontPath) {
    impl_->fontPath = fontPath;
    return *this;
}

App& App::resourceDirectory(const std::filesystem::path& directory) {
    impl_->resourceDirectory = directory;
    impl_->resourceDirectorySet = true;
    return *this;
}

Document& App::document() { return impl_->ensureDocument(); }

bool App::hasDocument() const noexcept { return impl_->document != nullptr; }

const std::string& App::windowTitle() const noexcept { return impl_->title; }

int App::windowWidth() const noexcept { return impl_->width; }

int App::windowHeight() const noexcept { return impl_->height; }

int App::run() { return impl_->run(); }

void App::quit() { impl_->quitRequested = true; }

} // namespace EntroUI
