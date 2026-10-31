# Styling

EntroUI uses a CSS-inspired stylesheet engine. It is not a complete CSS
implementation, but it supports the properties needed for expressive desktop
layouts.

```css
.container {
    padding: 20px;
    background: #ffffff;
    display: flex;
    flex-direction: column;
    gap: 16px;
}

.title {
    font-size: 24px;
    color: #222222;
}

button {
    padding: 10px 16px;
    background: #2563eb;
    color: #ffffff;
    border-radius: 6px;
}
```

## Selectors

| Selector | Example | Specificity |
|---|---|---|
| Element | `button` | 1 |
| Class | `.primary` | 10 |
| ID | `#submit` | 100 |
| Compound | `button.primary`, `#submit.large` | sum of parts |

Selector lists are supported: `.card, .panel { ... }`.

Descendant, child, pseudo-class and attribute selectors are not supported. Use
the classes `hover`, `active` and `focus` instead:

```css
button.hover { background: #3b82f6; }
button.active { background: #1d4ed8; }
input.focus { border-color: #2563eb; }
```

These classes are added and removed automatically by the event dispatcher.

## Cascade

1. Theme defaults (built-in control styling).
2. User stylesheet rules.
3. Inline styles.

Within a rule set, the higher specificity wins; on equal specificity, the later
rule wins. `!important` is parsed but not given priority.

## Properties

### Box model

| Property | Values |
|---|---|
| `width`, `height` | `px`, `%`, `auto` |
| `min-width`, `min-height`, `max-width`, `max-height` | `px`, `%` |
| `margin`, `padding` | 1-4 lengths |
| `margin-top/right/bottom/left`, `padding-*` | length |

Sizing uses **border-box** semantics: declared width and height include padding
and border.

### Colors

`color`, `background`, `background-color`, `border-color`.

Supported formats: `#rgb`, `#rgba`, `#rrggbb`, `#rrggbbaa`, named colors,
`rgb(r, g, b)` and `rgba(r, g, b, a)`. Alpha can be `0..255`, a percentage or a
decimal `0..1`.

### Typography

`font-size` (px), `font-weight` (`normal`, `bold`, `100..900`), `font-family`,
`text-align` (`left`, `center`, `right`), `line-height` (px or multiplier).

### Borders

`border` shorthand (`2px solid #333`), `border-width`, `border-radius`,
`border-color`.

### Layout

| Property | Values |
|---|---|
| `display` | `block`, `inline-block`, `flex`, `none` |
| `flex-direction` | `row`, `column` |
| `justify-content` | `start`, `center`, `end`, `space-between`, `space-around` |
| `align-items` | `start`, `center`, `end`, `stretch` |
| `gap` | length |

Flexbox is limited to single-line row/column layouts. Flex grow/shrink and
wrapping are not implemented in 1.0.0.

## Defaults

- `div`, `h1`, `h2`, `p`, `list`: `display: block`.
- `span`, `button`, `input`, `label`, `image`: `display: inline-block`.
- `h1`: 32px bold.
- `h2`: 24px bold.
- Body text: 16px, dark gray.

The built-in theme styles buttons and inputs, and provides hover/active/focus
variants. User rules override the theme at equal specificity because they are
applied later.

## Inheritance

`color`, `font-size`, `font-weight`, `font-family`, `text-align` and
`line-height` inherit from parent elements. Box, border and flex properties do
not inherit.

## Inline styles

```cpp
div().style("padding", "20px").style("background", "#ffffff")
```

```html
<div style="padding: 20px; background: #ffffff;"></div>
```

Inline styles have the highest priority.

## Validation

Unknown properties and invalid values produce warnings during stylesheet
parsing. Valid declarations in the same rule are still applied. Invalid
markup/stylesheets can be checked with:

```bash
./build/debug/entroui check ui/
```
