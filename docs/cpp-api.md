# C++ API

All public types live in the `EntroUI` namespace. Include the umbrella header:

```cpp
#include <entroui/entroui.hpp>
```

## App

```cpp
EntroUI::App app;
app.title("My Application");
app.size(900, 600);
app.background("#f8fafc");
app.mount(std::move(root));
app.stylesheet("ui/styles.css");
app.load("ui/app.eui");
app.font("/path/to/font.ttf");
app.resourceDirectory("assets");
int code = app.run();
```

| Method | Purpose |
|---|---|
| `title` | Set the window title |
| `size` | Set the initial window size |
| `background` | Set the window clear color |
| `mount` | Mount a C++ element tree |
| `mountComponent` | Mount a component as the root UI |
| `load` | Parse and mount a `.eui` file |
| `stylesheet` / `stylesheetText` | Load CSS |
| `font` | Set the default font file |
| `resourceDirectory` | Set the base directory for images and assets |
| `document()` | Access the mounted document |
| `run()` | Run the SDL event/render loop |
| `quit()` | Request loop shutdown |

`run()` throws if no root is mounted or if EntroUI was built without SDL.

## Elements

Factories return move-only `Element` values. Fluent methods return `Element&&`,
so chains can be assigned directly:

```cpp
auto page = div()
    .id("root")
    .className("container")
    .style("padding", "24px")
    .child(h1().text("Title"))
    .child(p().text("Body"))
    .child(button("Save").onClick([](Event&) {}));
```

Factories:

```cpp
div(), h1(), h2(), p(), span(), button(text), input(placeholder),
label(text), image(source), list(), textNode(text)
```

Identity and content:

```cpp
.id("name")
.className("a b")
.addClass("c")
.removeClass("c")
.toggleClass("selected", true)
.attr("data-id", "42")
.hasAttribute("disabled")
.attribute("data-id")
.removeAttribute("data-id")
.text("content")
.textContent()
.style("color", "#ffffff")
.style("color: #fff; padding: 8px;")
```

Tree operations:

```cpp
.appendChild(std::unique_ptr<Node>)
.child(Element&&)
.child(TextNode&&)
.child(std::unique_ptr<Node>)
.removeChild(const Node&)
.detachChild(const Node&)
.clearChildren()
.children()
.childAt(index)
.findById(id)
.findByClass(className)
.findByTag(tag)
.traverse(visitor)
.traverseElements(visitor)
.cloneElement()
```

Ownership is `std::unique_ptr` based. Parent pointers are non-owning. Moving an
element repairs its children's parent pointers and any text bindings.

## Events

```cpp
button("Save")
    .onClick([](Event& event) {})
    .onMouseEnter([](Event&) {})
    .onMouseLeave([](Event&) {})
    .onKeyDown([](Event&) {})
    .onFocus([](Event&) {})
    .onBlur([](Event&) {})
    .onChange([](Event&) {})
    .on(EventType::Click, [](Event&) {});
```

Events bubble from the target through its ancestors. Call
`event.stopPropagation()` to stop bubbling and `event.preventDefault()` to skip
default actions. `MouseEvent` carries `x`, `y`, `button` and `clicks`;
`KeyEvent` carries a translated key constant (`EntroUI::Keys`), scancode,
modifiers and repeat state.

## State

```cpp
auto count = state<int>(0);
auto name = state<std::string>("EntroUI");

count.set(count.get() + 1);
count.update([](int value) { return value + 1; });
auto subscription = count.subscribe([](const int& value) {});

h1().bindText(count);
p().bindText(name);
```

State changes mark the document dirty and update bound text immediately. Bindings
are cancelled automatically when their element is destroyed. State is not
thread-safe; mutate it on the UI thread.

## Components

```cpp
class Counter : public Component {
public:
    Element render() override {
        return div()
            .child(h1().bindText(count))
            .child(button("+1").onClick([this](Event&) {
                count.set(count.get() + 1);
                requestUpdate();
            }));
    }

    void onMount() override {}
    void onUnmount() override {}

private:
    State<int> count{0};
};

App app;
app.mountComponent(std::make_unique<Counter>());
```

`requestUpdate()` schedules a full re-render of the component root on the UI
thread. Nested components can be composed by returning their rendered elements
from `render()`. Functional components are ordinary functions returning
`Element`.

## Parsing and validation without a window

```cpp
Parser parser;
auto result = parser.parseFile("ui/app.eui");
for (const auto& diagnostic : result.diagnostics) {
    std::cerr << diagnostic.format() << '\n';
}

auto built = buildDocument(*result.value);
auto& document = *built.document;

computeDocumentStyles(document, defaultThemeStylesheet().get(), nullptr);

FixedTextMeasurer measurer;
LayoutTree layout;
layout.layout(document, 900.0F, 600.0F, measurer);

auto commands = buildRenderCommands(layout);
```

Everything up to render commands is testable without SDL or a visible window.

## Resources

```cpp
ResourceManager resources("assets");
auto image = resources.loadImage("logo.png");
auto size = resources.imageSize("logo.png");
```

Paths are resolved under the base directory; absolute paths and paths escaping
the base are rejected unless explicitly allowed. Image decoding uses stb_image.
Markup image sources resolve against the `.eui` file's directory by default.
