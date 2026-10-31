# Markup (`.eui`)

`.eui` files use an HTML-like syntax and are converted into the same
`Document` tree produced by the C++ DSL.

## Document structure

```html
<app>
    <window title="My Application" width="900" height="600">
        <div class="container">
            <h1>Hello</h1>
            <p>Description</p>
        </div>
    </window>
</app>
```

`<app>` is optional. When present, the first `<window>` child supplies the
application window metadata and becomes the rendered root. A top-level
`<window>` works as well. Any other top-level element is mounted directly.

Window attributes:

| Attribute | Meaning | Default |
|---|---|---|
| `title` | Window title | `EntroUI Application` |
| `width` | Window width in logical pixels | 900 |
| `height` | Window height in logical pixels | 600 |

`App::title`, `App::size`, `App::load` order matters: explicit values set on the
`App` before `load` win over markup window attributes.

## Elements

All tags are accepted. Unknown tags create a generic element so custom naming
schemes are possible. Initial built-in elements:

`div`, `h1`, `h2`, `p`, `span`, `button`, `input`, `label`, `image`, `list`,
`text` (text nodes are created from text content).

`input` and `image` are void tags and may be written without a closing tag.
All tags also support self-closing syntax.

## Attributes

```html
<button id="submit" class="primary large" disabled="true">Save</button>
<img src="assets/logo.png" />
```

- Attribute names and tag names are lowercased.
- Values may use single or double quotes.
- Boolean attributes without a value receive `"true"`.
- `id`, `class`, and `style` are handled specially by the element system.
- `style` accepts inline CSS declarations: `style="color: red; padding: 8px;"`.
- Entities are decoded: `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`, decimal
  and hexadecimal numeric entities.

## Text and whitespace

Text content is trimmed and whitespace runs are collapsed to a single space.
Whitespace-only nodes are discarded. This means indentation used for readability
does not become visible content. Whitespace between adjacent inline elements is
not preserved in this release.

## Comments

```html
<!-- This is ignored -->
<div>Visible</div>
```

`<!DOCTYPE ...>` declarations are ignored.

## Parser limits

- Maximum source size: 4 MiB.
- Maximum nesting depth: 256.
- The parser is iterative and the builder has an independent depth limit.
- Markup is treated as data. It cannot execute C++ code or load files outside
  the configured resource directory.

## Diagnostics

The parser reports:

- Missing closing tags.
- Mismatched closing tags.
- Unexpected closing tags.
- Missing or malformed attributes.
- Unterminated tags, comments and quoted values.
- Text outside the root element.
- Multiple root elements.
- Source size and nesting limits.

Each diagnostic includes file, line and column:

```text
ui/app.eui:12:9: error: Mismatched closing tag '</div>', expected '</span>'
```

Use `entroui check <path>` to validate a file or directory:

```bash
./build/debug/entroui check ui/
```

## Loading from C++

```cpp
App app;
app.load("ui/app.eui");
app.stylesheet("ui/styles.css");
```

`App::load` throws `std::runtime_error` with formatted diagnostics when the
markup contains errors. Warnings are logged through spdlog.
