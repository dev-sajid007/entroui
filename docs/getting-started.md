# Getting Started

## 1. Install prerequisites

On Fedora:

```bash
sudo dnf install gcc-c++ cmake ninja-build conan freetype-devel \
    libX11-devel libXext-devel libXrandr-devel libXcursor-devel \
    libXi-devel libXfixes-devel libXinerama-devel libxkbcommon-devel
```

On Debian/Ubuntu:

```bash
sudo apt install g++ cmake ninja-build conan libfreetype-dev \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
    libxfixes-dev libxinerama-dev libxkbcommon-dev
```

SDL3 and SDL3_ttf are fetched and built by CMake by default. If system SDL3
packages are installed, configure with `-DENTROUI_FETCH_SDL=OFF`.

## 2. Build EntroUI

```bash
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=20 -of build/debug
cmake --preset debug
cmake --build --preset debug -j
ctest --preset debug --output-on-failure
```

The helper script performs the same steps:

```bash
./scripts/build.sh debug
./scripts/test.sh debug
```

## 3. Create an application

Using the CLI:

```bash
./build/debug/entroui create my-app
./build/debug/entroui build my-app
./build/debug/entroui run my-app
```

Using a manual C++ program:

```cpp
#include <entroui/entroui.hpp>

using namespace EntroUI;

int main() {
    App app;
    app.title("My Application").size(900, 600).background("#f8fafc");

    app.mount(
        div()
            .style("padding", "32px")
            .child(h1().text("Welcome"))
            .child(p().text("This is an EntroUI application."))
            .child(button("Continue").onClick([](Event&) {
                // React to the click.
            })));

    return app.run();
}
```

## 4. Use markup

Create `ui/app.eui`:

```html
<app>
    <window title="My Application" width="900" height="600">
        <div class="container">
            <h1>Welcome</h1>
            <p>This is an EntroUI application.</p>
            <button id="continue">Continue</button>
        </div>
    </window>
</app>
```

Create `ui/styles.css`:

```css
.container {
    padding: 32px;
}

h1 {
    font-size: 36px;
    color: #0f172a;
}

button {
    background: #2563eb;
    color: #ffffff;
    padding: 10px 16px;
    border-radius: 8px;
}
```

Load both files:

```cpp
App app;
app.load("ui/app.eui");
app.stylesheet("ui/styles.css");
return app.run();
```

## 5. Add events and state

```cpp
auto count = state<int>(0);

auto page = div()
    .style("padding", "32px")
    .child(h1().bindText(count))
    .child(button("Increment").onClick([&](Event&) {
        count.set(count.get() + 1);
    }));

app.mount(std::move(page));
```

## 6. Add form controls

```cpp
auto name = Widgets::textInput("Your name");
name.onChange([](Event& event) {
    auto& input = static_cast<Element&>(*event.target);
    // input.value()
});

auto remember = Widgets::checkbox("Remember me", true);
```

## 7. Look at the examples

The repository includes six examples:

- `hello-world`
- `basic-elements` (markup + CSS)
- `layouts` (flex layout)
- `buttons` (events and reactive state)
- `forms` (text input and checkbox)
- `components` (component lifecycle)

Run them from `build/debug/examples` or `build/release/examples`.
