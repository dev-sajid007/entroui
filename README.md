# EntroUI

EntroUI is a lightweight, declarative C++20 desktop UI framework with an HTML-like
markup language (`.eui`) and a native C++ DSL. Both interfaces build the same
internal UI tree.

```cpp
#include <entroui/entroui.hpp>

using namespace EntroUI;

int main() {
    App app;
    app.title("Hello EntroUI").size(900, 600);
    app.mount(
        div()
            .style("padding", "40px")
            .child(h1().text("Hello EntroUI"))
            .child(button("Click Me").onClick([](Event&) {
                // handle the click
            })));
    return app.run();
}
```

```html
<app>
    <window title="Hello EntroUI" width="900" height="600">
        <div class="container">
            <h1>Hello EntroUI</h1>
            <button id="submit">Click Me</button>
        </div>
    </window>
</app>
```

## Features

- C++20 declarative DSL and `.eui` markup producing one UI tree.
- DOM-like element tree with IDs, classes, attributes, traversal and lookup.
- CSS-inspired stylesheet engine with element, class, ID and compound selectors.
- Block, inline and flexbox-inspired layout with padding, margin, borders, gap,
  alignment, justification, percentages, flex grow/shrink/wrap and min/max
  constraints.
- Text measuring and wrapping, font size/weight, line height and text alignment.
- Mouse, keyboard, focus, hover and text input events with bubbling, caret
  placement, selection and clipboard shortcuts.
- Reactive `State<T>` bindings and a component lifecycle.
- SDL3 windowing, software rendering and SDL_ttf text.
- Built-in widgets: button, label, text input, checkbox, image, container and
  scroll container.
- Unit and integration tests that do not require a visible window.

## Requirements

- Linux (primary), C++20 compiler, CMake 3.25+, Ninja.
- Conan 2 for the development build (GoogleTest, spdlog).
- SDL3 and SDL3_ttf are fetched and built from source by default.
- System FreeType development headers for SDL3_ttf.
- X11 or Wayland development headers for the SDL window backend.

## Build

```bash
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=20 -of build/debug
cmake --preset debug
cmake --build --preset debug -j
ctest --preset debug --output-on-failure
```

Or use the helper scripts:

```bash
./scripts/build.sh debug
./scripts/test.sh debug
```

Release build:

```bash
./scripts/build.sh release
```

If SDL development packages are already installed, disable the source fetch:

```bash
cmake --preset debug -DENTROUI_FETCH_SDL=OFF
```

## Run the examples

```bash
./build/debug/examples/entroui-hello
./build/debug/examples/entroui-basic-elements
./build/debug/examples/entroui-layouts
./build/debug/examples/entroui-buttons
./build/debug/examples/entroui-forms
./build/debug/examples/entroui-components
```

## CLI

The CLI is built as the `entroui` binary (CMake target `entroui-cli`):

```bash
./build/debug/entroui create my-app
./build/debug/entroui build my-app
./build/debug/entroui run my-app
./build/debug/entroui check my-app/ui
./build/debug/entroui version
```

## Documentation

- [Getting started](docs/getting-started.md)
- [Architecture](docs/architecture.md)
- [Markup](docs/markup.md)
- [C++ API](docs/cpp-api.md)
- [Styling](docs/styling.md)
- [Components](docs/components.md)
- [CLI](docs/cli.md)
- [Roadmap](docs/roadmap.md)

## Status

Version 1.1.0 adds inline formatting, flex grow/shrink/wrap and text selection
on top of the 1.0.0 production foundation.

## License

MIT. See [LICENSE](LICENSE).
