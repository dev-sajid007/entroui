# EntroUI — Complete Project Specification

**Version:** 1.0.0  
**Status:** Initial Development  
**Language:** C++20  
**Project Type:** Native Desktop UI Framework  
**Primary Platform:** Linux  
**Future Platforms:** Windows, macOS

---

# 1. Project Overview

## 1.1 Introduction

EntroUI is a modern, lightweight, extensible C++ Desktop UI Framework designed to provide developers with an HTML-like interface development experience.

The framework will allow developers to build desktop applications using two programming interfaces:

1. HTML-like markup using `.eui` files.
2. Native C++ declarative UI using the EntroUI DSL.

EntroUI will provide its own:

- UI Element System
- DOM / UI Tree
- Markup Parser
- Component System
- Layout Engine
- Styling Engine
- Event System
- State Management
- Rendering Engine
- Window Management
- Application Lifecycle

The initial implementation will use SDL3 as the platform and rendering backend foundation. Core UI architecture, parsing, layout, and component logic will remain independent of SDL3.

## 1.2 Vision

Build a C++ UI framework that makes desktop application development easier, declarative, and extensible.

Developers should be able to write UI code similar to HTML:

```html
<div class="container">
    <h1>Hello EntroUI</h1>

    <p>Welcome to my application.</p>

    <button id="submit">
        Click Me
    </button>
</div>
```

Or use the C++ API:

```cpp
auto page = EntroUI::div()
    .className("container")
    .child(
        EntroUI::h1()
            .text("Hello EntroUI")
    )
    .child(
        EntroUI::p()
            .text("Welcome to my application.")
    );
```

Both interfaces should produce the same internal UI tree representation.

## 1.3 Design Principles

- C++20-first development.
- Declarative UI development.
- Separation of concerns.
- Backend-independent UI architecture.
- Lightweight dependencies.
- Extensible component architecture.
- Type-safe C++ APIs.
- Predictable memory ownership.
- Testable individual modules.
- Developer-friendly documentation.

---

# 2. Project Goals

## 2.1 Primary Goals

1. Create a reusable C++ UI library.
2. Support HTML-like markup.
3. Support a C++ declarative API.
4. Implement a DOM-like element tree.
5. Create a custom layout engine.
6. Create a CSS-inspired styling system.
7. Implement event handling.
8. Implement reactive state management.
9. Render UI in native desktop windows.
10. Provide a component-based development experience.

## 2.2 Non-Goals for Version 1

The following features are outside the initial release scope:

- Full browser engine.
- HTML5 compatibility.
- JavaScript execution engine.
- Web application hosting.
- Full CSS specification compatibility.
- WebAssembly runtime.
- Native mobile application support.
- Distributed UI rendering.

EntroUI is a native desktop framework, not a web browser.

---

# 3. Technology Stack

| Component | Technology |
|---|---|
| Programming Language | C++20 |
| Build System | CMake |
| Package Manager | Conan 2 |
| Compiler | GCC / Clang |
| Window Management | SDL3 |
| Rendering Backend | SDL3 Renderer |
| Markup Parser | Custom C++ Parser |
| Layout Engine | Custom Layout Engine |
| Styling | Custom CSS-inspired Engine |
| Testing | GoogleTest |
| Logging | spdlog |
| Formatting | clang-format |
| Static Analysis | clang-tidy |
| Documentation | Doxygen |
| CI | GitHub Actions |

### Technology Decisions

**C++20**

Use modern C++ features such as:

- `std::unique_ptr`
- `std::shared_ptr`
- `std::optional`
- `std::variant`
- Concepts
- Ranges
- Structured bindings

**CMake**

CMake will manage library builds, examples, tests, and installation.

**Conan 2**

Conan will manage external dependencies.

**SDL3**

SDL3 will provide:

- Window creation.
- Input events.
- Keyboard events.
- Mouse events.
- Rendering backend integration.
- Platform abstraction.

SDL3 is an initial backend choice, not a dependency of the UI tree or parser.

---

# 4. High-Level Architecture

EntroUI will use a layered architecture.

```text
+------------------------------------------+
|             User Application             |
|                                          |
|        main.cpp / app.eui / styles.css    |
+--------------------+---------------------+
                     |
                     v
+------------------------------------------+
|             Developer API                |
|                                          |
|          C++ DSL / Markup API             |
+--------------------+---------------------+
                     |
                     v
+------------------------------------------+
|              Markup Parser                |
|                                          |
|          Lexer / Parser / AST             |
+--------------------+---------------------+
                     |
                     v
+------------------------------------------+
|               UI Core                    |
|                                          |
|        Node / Element / Text Node         |
|                                          |
|             UI Tree                       |
+--------------------+---------------------+
                     |
          +----------+----------+
          |                     |
          v                     v
+-------------------+  +-------------------+
|  Styling Engine   |  |   Event System    |
|                   |  |                   |
| CSS-like Rules    |  | Event Dispatcher  |
+---------+---------+  +-------------------+
          |
          v
+------------------------------------------+
|              Layout Engine                |
|                                          |
|       Box Model / Flex Layout             |
+--------------------+---------------------+
                     |
                     v
+------------------------------------------+
|             Render Tree                  |
|                                          |
|       Paint Commands / Draw Objects       |
+--------------------+---------------------+
                     |
                     v
+------------------------------------------+
|             Renderer                     |
|                                          |
|              SDL3 Backend                |
+--------------------+---------------------+
                     |
                     v
+------------------------------------------+
|            Native Desktop Window          |
+------------------------------------------+
```

## 4.1 Architecture Rules

- Parser must not depend on SDL3.
- DOM must not depend on the rendering backend.
- Styling must not depend on SDL3.
- Layout must be independently testable.
- Event logic must be separated from platform input handling.
- Rendering backends must implement a common renderer interface.
- C++ DSL and markup must generate the same UI tree.

---

# 5. Dual UI Development Syntax

Supporting two syntax styles is a core requirement.

## 5.1 Markup Syntax

File extension: `.eui`

Example:

```html
<app>
    <window title="EntroUI Application"
            width="900"
            height="600">

        <div class="container">

            <h1>Hello EntroUI</h1>

            <p>
                My first C++ desktop application.
            </p>

            <button id="submit">
                Click Me
            </button>

        </div>

    </window>
</app>
```

The `.eui` parser will convert markup into an internal AST and then construct the UI tree.

## 5.2 C++ DSL Syntax

Example:

```cpp
#include <entroui/entroui.hpp>

using namespace EntroUI;

int main() {

    App app;

    auto page = div()
        .className("container")
        .child(
            h1().text("Hello EntroUI")
        )
        .child(
            p().text("My first desktop application.")
        )
        .child(
            button()
                .id("submit")
                .text("Click Me")
        );

    app.mount(page);

    app.run();

    return 0;
}
```

## 5.3 Important C++ Syntax Decision

HTML-like syntax cannot be written directly inside an ordinary `.cpp` file as valid standard C++20.

Therefore, EntroUI will initially support:

- `.eui` files through a runtime or build-time parser.
- Native C++ DSL through ordinary C++20 APIs.

A future optional compiler tool called `entroui-compiler` may transform JSX-like source into standard C++ code.

Example future syntax:

```jsx
const page = (
    <div class="container">
        <h1>Hello EntroUI</h1>
    </div>
);
```

This transformation will be implemented using a separate compiler/code-generation layer. It is not a requirement for the first release.

---

# 6. Core Module Specifications

## 6.1 Application Module

Responsible for:

- Application initialization.
- Window lifecycle.
- Root UI mounting.
- Event loop.
- Rendering lifecycle.
- Shutdown handling.

Proposed API:

```cpp
EntroUI::App app;

app.title("EntroUI Application");
app.size(900, 600);
app.mount(page);
app.run();
```

## 6.2 Element Module

Responsible for creating UI elements.

Initial elements:

| Element | Purpose |
|---|---|
| div | Container |
| h1 | Heading |
| h2 | Heading |
| p | Paragraph |
| span | Inline text |
| button | Interactive button |
| input | Input control |
| label | Label |
| image | Image |
| list | List |
| text | Text node |

Proposed API:

```cpp
auto element = EntroUI::div()
    .id("container")
    .className("main")
    .text("Hello");
```

## 6.3 DOM / UI Tree Module

The UI Tree represents the application's interface.

Node types:

```cpp
enum class NodeType {
    Element,
    Text
};
```

Conceptual structure:

```text
Root
 |
 +-- div
      |
      +-- h1
      |    |
      |    +-- Text: Hello
      |
      +-- p
      |    |
      |    +-- Text: Welcome
      |
      +-- button
           |
           +-- Text: Click Me
```

Requirements:

- Append child.
- Remove child.
- Find by ID.
- Find by class.
- Find by tag.
- Traverse tree.
- Replace child.
- Clear children.
- Parent-child relationship.
- Safe ownership and lifetime management.

Recommended ownership model:

- Parent owns child nodes using `std::unique_ptr`.
- Parent references should be non-owning.
- Avoid unnecessary `std::shared_ptr` ownership cycles.

## 6.4 Markup Parser

The parser will convert `.eui` source into UI nodes.

Architecture:

```text
Source File
    |
    v
Lexer
    |
    v
Tokens
    |
    v
Parser
    |
    v
AST
    |
    v
UI Tree Builder
    |
    v
DOM
```

Initial supported syntax:

- Opening tags.
- Closing tags.
- Self-closing tags.
- Attributes.
- Text content.
- Nested elements.
- Comments.
- Whitespace handling.

Example:

```html
<div class="card">
    <h1>Title</h1>
    <p>Description</p>
</div>
```

Expected parser errors:

- Missing closing tag.
- Invalid attribute syntax.
- Unexpected closing tag.
- Invalid nesting.
- Unknown or malformed token.

Parser must provide useful source line and column information.

## 6.5 Styling Engine

EntroUI will implement a CSS-inspired styling system.

Example:

```css
.container {
    width: 500px;
    padding: 20px;
    background: #ffffff;
}

.title {
    font-size: 24px;
    color: #222222;
}

button {
    padding: 10px 16px;
    background: #2563eb;
    color: #ffffff;
}
```

Initial supported properties:

### Box Model

- width
- height
- min-width
- min-height
- max-width
- max-height
- margin
- padding

### Colors

- color
- background
- border-color

### Typography

- font-size
- font-weight
- text-align
- line-height

### Borders

- border
- border-width
- border-radius

### Layout

- display
- flex-direction
- justify-content
- align-items
- gap

Initial selector support:

- Element selector.
- Class selector.
- ID selector.

Advanced CSS selectors and full CSS compatibility are not required.

## 6.6 Layout Engine

The layout engine calculates the position and size of UI elements.

Initial layout support:

- Block layout.
- Horizontal layout.
- Vertical layout.
- Flexbox-inspired layout.
- Padding.
- Margin.
- Gap.
- Alignment.
- Width and height constraints.

Example:

```css
.container {
    display: flex;
    flex-direction: column;
    gap: 16px;
    padding: 20px;
}
```

Layout calculations must be independent of rendering.

## 6.7 Event System

Responsible for user interactions.

Initial events:

- Click.
- Mouse enter.
- Mouse leave.
- Mouse move.
- Mouse down.
- Mouse up.
- Key down.
- Key up.
- Input change.
- Focus.
- Blur.

Example:

```cpp
auto submit = button()
    .text("Submit")
    .onClick([] {
        std::cout << "Button clicked!\n";
    });
```

Event architecture:

```text
SDL3 Input Event
       |
       v
Platform Event Adapter
       |
       v
EntroUI Event Dispatcher
       |
       v
Target UI Element
       |
       v
Registered Callback
```

## 6.8 Component System

Developers should be able to create reusable UI components.

Example:

```cpp
Element createCard(std::string title) {

    return div()
        .className("card")
        .child(
            h2().text(title)
        );
}
```

Future component API:

```cpp
Component ProfileCard() {

    return Component(
        div()
            .child(h1().text("Sajid"))
            .child(p().text("Developer"))
    );
}
```

Requirements:

- Reusable components.
- Component properties.
- Nested components.
- Component lifecycle.
- Component-local state.

## 6.9 State Management

EntroUI will eventually provide reactive state management.

Conceptual API:

```cpp
auto count = state<int>(0);

auto counter = div()
    .child(
        h1().bindText(count)
    )
    .child(
        button()
            .text("Increment")
            .onClick([&] {
                count.set(count.get() + 1);
            })
    );
```

When state changes, dependent UI should update without rebuilding the entire application unnecessarily.

The first implementation may use explicit subscriptions and dirty-node tracking.

## 6.10 Rendering Engine

The renderer converts the calculated render tree into visual output.

Initial backend:

SDL3.

Responsibilities:

- Draw rectangles.
- Draw borders.
- Draw text.
- Draw images.
- Draw buttons.
- Draw backgrounds.
- Handle clipping.
- Handle scaling.

Architecture:

```cpp
class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void beginFrame() = 0;
    virtual void drawRect(const Rect& rect, const Color& color) = 0;
    virtual void drawText(const TextCommand& command) = 0;
    virtual void endFrame() = 0;
};
```

SDL3 implementation:

```cpp
class SDLRenderer : public Renderer {
    // SDL3-specific rendering implementation
};
```

Keep renderer interfaces independent of SDL3.

Text rendering will require a suitable font backend, selected and integrated during the rendering milestone.

---

# 7. Proposed Repository Structure

```text
entroui/
│
├── CMakeLists.txt
├── CMakePresets.json
├── conanfile.py
├── README.md
├── LICENSE
├── .gitignore
├── .clang-format
├── .clang-tidy
│
├── include/
│   └── entroui/
│       │
│       ├── entroui.hpp
│       ├── app.hpp
│       ├── config.hpp
│       │
│       ├── core/
│       │   ├── node.hpp
│       │   ├── element.hpp
│       │   ├── text_node.hpp
│       │   ├── document.hpp
│       │   └── component.hpp
│       │
│       ├── parser/
│       │   ├── lexer.hpp
│       │   ├── token.hpp
│       │   ├── parser.hpp
│       │   └── ast.hpp
│       │
│       ├── style/
│       │   ├── style.hpp
│       │   ├── stylesheet.hpp
│       │   ├── selector.hpp
│       │   └── computed_style.hpp
│       │
│       ├── layout/
│       │   ├── layout_engine.hpp
│       │   ├── layout_node.hpp
│       │   └── box.hpp
│       │
│       ├── events/
│       │   ├── event.hpp
│       │   ├── event_target.hpp
│       │   └── event_dispatcher.hpp
│       │
│       ├── state/
│       │   ├── state.hpp
│       │   └── signal.hpp
│       │
│       └── renderer/
│           ├── renderer.hpp
│           ├── render_tree.hpp
│           └── render_command.hpp
│
├── src/
│   ├── core/
│   ├── parser/
│   ├── style/
│   ├── layout/
│   ├── events/
│   ├── state/
│   ├── renderer/
│   │   └── sdl/
│   └── platform/
│       └── sdl/
│
├── compiler/
│   └── README.md
│
├── examples/
│   ├── hello-world/
│   ├── basic-elements/
│   ├── layouts/
│   ├── buttons/
│   ├── forms/
│   └── components/
│
├── ui/
│   └── app.eui
│
├── tests/
│   ├── core/
│   ├── parser/
│   ├── style/
│   ├── layout/
│   ├── events/
│   └── state/
│
├── docs/
│   ├── architecture.md
│   ├── getting-started.md
│   ├── markup.md
│   ├── cpp-api.md
│   ├── styling.md
│   ├── components.md
│   └── roadmap.md
│
└── scripts/
    ├── build.sh
    └── test.sh
```

This is the target repository structure. Modules will be added incrementally as milestones are implemented.

---

# 8. Build System

## 8.1 CMake Targets

The project will expose these targets:

| Target | Purpose |
|---|---|
| `entroui` | Main library |
| `EntroUI::entroui` | Public CMake alias |
| `entroui-hello` | Hello World example |
| `entroui-parser` | Parser example |
| `entroui-tests` | Automated tests |
| `entroui-compiler` | Future markup compiler |

## 8.2 Build Commands

```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build

ctest --test-dir build --output-on-failure
```

Release build:

```bash
cmake -S . -B build-release -G Ninja \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-release
```

---

# 9. Public API Design

The public API should remain simple and consistent.

## 9.1 Application

```cpp
EntroUI::App app;

app.title("My Application");
app.size(900, 600);
app.mount(root);
app.run();
```

## 9.2 Elements

```cpp
auto root = EntroUI::div()
    .id("root")
    .className("container")
    .child(
        EntroUI::h1().text("Hello")
    );
```

## 9.3 Markup Loading

```cpp
EntroUI::App app;

app.load("ui/app.eui");
app.run();
```

## 9.4 Styling

```cpp
app.stylesheet("ui/styles.css");
```

## 9.5 Events

```cpp
button()
    .text("Save")
    .onClick([] {
        std::cout << "Saved!\n";
    });
```

These APIs define the intended public interface. Each will become available when its corresponding implementation milestone is completed.

---

# 10. Performance and Memory Management

Performance is an important design requirement.

## 10.1 Memory

- Prefer RAII.
- Use `std::unique_ptr` for tree ownership.
- Avoid unnecessary allocations.
- Avoid circular ownership.
- Release resources during application shutdown.
- Use stable node identifiers where needed.

## 10.2 Rendering

- Avoid unnecessary redraws.
- Track dirty nodes.
- Separate layout invalidation from paint invalidation.
- Cache computed styles where practical.
- Cache reusable rendering resources.
- Avoid rebuilding the entire render tree for every small state update.

## 10.3 Threading

Initial release:

- UI operations run on the main thread.
- Rendering runs on the main thread.
- Background tasks may perform non-UI work.
- UI mutations from other threads must be dispatched to the UI thread.

Thread-safe reactive updates are a future enhancement.

---

# 11. Security and Reliability

The markup parser must treat markup as data, not executable code.

Requirements:

- Validate malformed markup.
- Prevent uncontrolled recursive parsing.
- Define maximum parser nesting depth.
- Validate resource paths.
- Avoid arbitrary file access from markup.
- Never execute arbitrary C++ code from `.eui`.
- Avoid unsafe raw pointers.
- Provide clear error reporting.

Resource loading should use an explicit application resource policy.

---

# 12. Testing Strategy

Use GoogleTest.

## Unit Tests

### Core

- Element creation.
- Child insertion.
- Child removal.
- Node lookup.
- Tree traversal.

### Parser

- Basic element parsing.
- Nested element parsing.
- Attribute parsing.
- Text parsing.
- Malformed markup.
- Parser diagnostics.

### Styling

- CSS declaration parsing.
- Selector matching.
- Style inheritance where supported.
- Computed style generation.

### Layout

- Width and height.
- Padding.
- Margin.
- Flex direction.
- Alignment.
- Gap.

### Events

- Event registration.
- Event dispatch.
- Event target.
- Callback execution.

### Integration Tests

- Load `.eui`.
- Build UI tree.
- Apply styles.
- Calculate layout.
- Generate render commands.

Tests must not require a visible desktop window unless specifically testing the SDL3 integration.

---

# 13. Development Roadmap

Development will be divided into milestones.

## Phase 1 — Core Foundation

### Milestone 01: Project Setup

- Create repository.
- Configure CMake.
- Set C++20.
- Create public headers.
- Create library target.
- Build Hello World.
- Verify C++ DSL.

**Deliverable:** Working C++ library foundation.

### Milestone 02: DOM / Element Tree

- Implement Node.
- Implement Element.
- Implement TextNode.
- Implement Document.
- Parent-child relationships.
- Append and remove.
- Find by ID.
- Find by class.
- Tree traversal.

**Deliverable:** Complete initial UI tree system.

### Milestone 03: Markup Parser

- Lexer.
- Tokens.
- AST.
- Parser.
- Attribute parsing.
- Nested elements.
- Text nodes.
- Parser diagnostics.
- `.eui` file loading.

**Deliverable:** Working HTML-like markup parser.

### Milestone 04: UI Tree Builder

- AST-to-DOM conversion.
- C++ DSL integration.
- Document mounting.
- Node lookup.
- Tree validation.

**Deliverable:** Both syntax interfaces produce the same UI tree.

## Phase 2 — Desktop Platform

### Milestone 05: SDL3 Window Management

- Initialize SDL3.
- Create window.
- Configure dimensions.
- Handle application lifecycle.
- Handle resize.
- Handle close event.
- Create main event loop.

**Deliverable:** Native desktop window.

### Milestone 06: Rendering Foundation

- Renderer interface.
- SDL3 renderer backend.
- Render commands.
- Rectangle rendering.
- Background rendering.
- Basic clipping.

**Deliverable:** First visible EntroUI elements.

### Milestone 07: Text Rendering

- Font integration.
- Text measurement.
- Text drawing.
- Font size.
- Font weight.
- Text alignment.

**Deliverable:** Visible headings and paragraphs.

## Phase 3 — Styling and Layout

### Milestone 08: Styling Engine

- Stylesheet parser.
- Element selectors.
- Class selectors.
- ID selectors.
- Color.
- Background.
- Borders.
- Typography.
- Computed styles.

**Deliverable:** CSS-inspired styling.

### Milestone 09: Layout Engine

- Box model.
- Width and height.
- Padding.
- Margin.
- Block layout.
- Flex layout.
- Alignment.
- Gap.
- Layout invalidation.

**Deliverable:** Structured desktop layouts.

## Phase 4 — Interaction

### Milestone 10: Event System

- SDL3 event adapter.
- Mouse events.
- Keyboard events.
- Click handling.
- Event dispatcher.
- Callback registration.
- Focus management.

**Deliverable:** Interactive UI controls.

### Milestone 11: Components

- Component abstraction.
- Component properties.
- Component rendering.
- Nested components.
- Component reuse.

**Deliverable:** Reusable UI components.

### Milestone 12: Reactive State

- State container.
- Signals.
- Subscriptions.
- Dirty tracking.
- UI updates.
- Reactive text binding.

**Deliverable:** Reactive UI system.

## Phase 5 — Production Foundation

### Milestone 13: Common Widgets

- Button.
- Label.
- Text input.
- Checkbox.
- Image.
- Container.
- Scroll container.

**Deliverable:** Basic desktop widget collection.

### Milestone 14: Resource Management

- Image loading.
- Font loading.
- Resource cache.
- Resource lifecycle.
- Error handling.

**Deliverable:** Reliable asset management.

### Milestone 15: Developer Tooling

- CLI build helper.
- Project templates.
- Markup validation.
- Error diagnostics.
- Documentation.
- Examples.

**Deliverable:** Developer-friendly framework workflow.

### Milestone 16: Testing and Release

- Unit test coverage.
- Integration tests.
- Performance profiling.
- Memory checks.
- Linux release.
- Installation support.
- Versioning.
- Release documentation.

**Deliverable:** EntroUI v1.0.0.

---

# 14. CLI Design

Future CLI tool:

```bash
entroui create my-app
```

Project commands:

```bash
entroui build
entroui run
entroui test
entroui check
entroui version
```

Expected generated project:

```text
my-app/
├── CMakeLists.txt
├── conanfile.py
├── src/
│   └── main.cpp
├── ui/
│   ├── app.eui
│   └── styles.css
└── assets/
```

The CLI will be developed after the core framework stabilizes.

---

# 15. Versioning Strategy

Follow Semantic Versioning.

Format:

```text
MAJOR.MINOR.PATCH
```

Examples:

- `0.1.0` — Initial core.
- `0.2.0` — DOM and parser.
- `0.3.0` — Desktop rendering.
- `0.4.0` — Styling and layout.
- `0.5.0` — Events and components.
- `0.6.0` — Reactive state.
- `1.0.0` — Initial stable framework.

Breaking changes before v1.0 should be documented.

---

# 16. Definition of Done

EntroUI v1.0 will be considered ready when:

- [ ] C++20 library builds on supported Linux distributions.
- [ ] CMake installation works.
- [ ] C++ DSL works.
- [ ] `.eui` parser works.
- [ ] Both syntax interfaces generate equivalent UI trees.
- [ ] DOM supports basic tree operations.
- [ ] SDL3 window works.
- [ ] Rendering works.
- [ ] Text is rendered correctly.
- [ ] Styling system works.
- [ ] Layout engine works.
- [ ] Mouse and keyboard events work.
- [ ] Reusable components work.
- [ ] Basic reactive state works.
- [ ] Automated tests pass.
- [ ] Documentation is available.
- [ ] At least five working example applications are available.

---

# 17. Future Vision

After v1.0, potential extensions include:

- Windows and macOS backends.
- Vulkan rendering backend.
- OpenGL rendering backend.
- Advanced layout algorithms.
- Animation system.
- Theme engine.
- Accessibility.
- Advanced developer tools.
- Hot reload for `.eui` files.
- JSX-like C++ compiler transformation.
- Visual UI designer.
- Component marketplace.

These are future possibilities, not v1.0 commitments.

---

# 18. Final Project Direction

EntroUI will be developed as an independent C++ Desktop UI Framework.

The framework will own its UI tree, markup parser, styling, layout, event handling, and component architecture.

SDL3 will initially provide the native platform and rendering integration.

The most important architectural requirement is that the C++ DSL and `.eui` markup interfaces must share one internal UI representation.

Development will proceed incrementally, starting with the C++20 foundation and ending with a stable, reusable desktop UI framework.

**Project:** EntroUI  
**Version:** 1.0.0 Specification  
**Status:** Ready for milestone-based implementation

