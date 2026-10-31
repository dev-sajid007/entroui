# Architecture

EntroUI is organized as a layered framework. The core is independent of the
windowing and rendering backend.

```text
Application (C++ DSL / .eui)
        |
        v
Parser (lexer -> tokens -> AST)  --->  Document / UI tree
        |                                      |
        +--------------------------------------+
                                               |
                 +-----------------------------+------------------+
                 |                             |                  |
                 v                             v                  v
          Styling engine                Layout engine        Event system
                 |                             |                  |
                 +-----------------------------+------------------+
                                               |
                                               v
                                       Render command tree
                                               |
                                               v
                                        Renderer (SDL3)
                                               |
                                               v
                                        Native window
```

## Modules

| Module | Location | Responsibility |
|---|---|---|
| Core | `include/entroui/core`, `src/core` | Nodes, elements, text nodes, document, components |
| Parser | `include/entroui/parser`, `src/parser` | `.eui` lexer, parser, AST and DOM builder |
| Style | `include/entroui/style`, `src/style` | CSS parser, selectors, cascade, computed styles |
| Layout | `include/entroui/layout`, `src/layout` | Box model, block/flex layout, text wrapping, hit testing |
| Events | `include/entroui/events`, `src/events` | Event payloads, hit testing, dispatch, focus |
| State | `include/entroui/state` | Reactive state and text bindings |
| Renderer | `include/entroui/renderer`, `src/renderer` | Render command model and renderer interface |
| SDL backend | `src/platform/sdl`, `src/renderer/sdl` | Window, input adapter, fonts, images, drawing |
| Resources | `include/entroui/resource`, `src/resource` | Safe paths, image loading, asset access |
| Widgets | `include/entroui/widgets`, `src/widgets` | Convenience widgets built from elements |
| CLI | `tools/entroui_cli.cpp` | Project creation, build/run/test and markup validation |

## UI tree and ownership

- `Document` owns the root `Element` with `std::unique_ptr`.
- `Element` owns children with `std::unique_ptr<Node>`.
- Parent pointers are non-owning and are maintained by the tree operations.
- Text nodes are leaf nodes. An element's readable text is concatenated with
  `textContent()`.
- Every node receives a stable `uid` when attached to a document.
- Elements can be moved before or after mounting; move operations repair child
  parent pointers and reactive binding targets.
- `clone()` performs a deep structural copy without copying event listeners or
  reactive bindings.

## Dirty tracking

`Document` tracks three flags:

- `styleDirty`: styles must be recomputed.
- `layoutDirty`: layout must be recomputed.
- `paintDirty`: render commands must be rebuilt.

Changing an element's classes, ID or attributes invalidates style, layout and
paint. Changing text invalidates layout and paint. Hover/focus/active classes and
state bindings invalidate the appropriate flags. The application loop skips
style, layout and rendering work when the corresponding flag is clean, and skips
presenting a frame when nothing is dirty.

## Styling pipeline

1. Start from inherited properties (color, typography, text alignment, line
   height, font family).
2. Apply per-tag defaults (display mode and heading sizes).
3. Collect matching theme rules, then user rules, using specificity and source
   order.
4. Apply inline styles last.
5. Cache the result on the element until style is invalidated.

## Layout pipeline

1. Build a `LayoutBox` tree from the element tree and computed styles.
2. Resolve box edges (margin, border, padding).
3. Resolve widths and heights from pixels, percentages, auto sizing and
   min/max constraints.
4. Lay out children using block or flex rules.
5. Lay out text nodes into wrapped lines using an injected `TextMeasurer`.
6. Produce absolute border, padding and content rectangles.
7. Apply scroll offsets and clipping for scrollable containers.

Layout is entirely independent of SDL. Tests use `FixedTextMeasurer`; the
application injects an SDL_ttf measurer.

## Event pipeline

```text
SDL event
   |
   v
App event adapter (key/button translation)
   |
   v
EventDispatcher (hit test, hover path, focus)
   |
   v
Target element -> bubble through ancestors
   |
   v
Registered callbacks; default actions (click, checkbox, focus, text editing)
```

Hover, active and focus states are represented as `hover`, `active` and `focus`
classes so they participate in the normal stylesheet cascade.

## Rendering pipeline

The layout tree is converted into a list of paint commands: filled rectangles,
borders, text lines, images and clip push/pop commands. The list is testable
without SDL. The SDL renderer consumes the same list through the abstract
`Renderer` interface and draws with SDL3 and SDL_ttf.

## Backend independence

- Parser, DOM, styling, layout and event dispatch have no SDL dependency.
- The renderer interface and render commands have no SDL types.
- SDL-specific code lives under `src/platform/sdl` and `src/renderer/sdl`.
- Building with `ENTROUI_BUILD_SDL=OFF` produces the core library without a
  window backend; callbacks, tests and headless tools continue to work.
