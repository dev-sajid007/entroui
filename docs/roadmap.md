# Roadmap

The EntroUI 1.1.0 implementation covers the following milestones.

## Version 1.1.0 - Layout and text fidelity

- [x] Inline formatting context: text runs, `span`, and inline-block elements on
      shared wrapped lines.
- [x] Inline whitespace preservation and line-level text alignment.
- [x] Flex grow, shrink, basis, row wrapping, align-self, align-content,
      row-gap and column-gap.
- [x] Text input caret placement by click, selection, clipboard copy/cut/paste
      and keyboard selection shortcuts.
- [x] Additional layout, event and integration tests.

## Phase 1 - Core foundation

- [x] Milestone 01: Project setup, CMake, C++20, public headers, library target.
- [x] Milestone 02: Node, Element, TextNode, Document, ownership, lookup,
      traversal, append/remove/replace.
- [x] Milestone 03: Lexer, tokens, AST, parser, attributes, nesting, text,
      diagnostics, `.eui` loading.
- [x] Milestone 04: AST-to-DOM builder, DSL integration, mounting, validation.

## Phase 2 - Desktop platform

- [x] Milestone 05: SDL3 window lifecycle, resize, close, event loop.
- [x] Milestone 06: Renderer interface, render commands, SDL3 backend,
      rectangles, borders, backgrounds, clipping.
- [x] Milestone 07: SDL_ttf font integration, measurement, drawing, size,
      weight, alignment.

## Phase 3 - Styling and layout

- [x] Milestone 08: Stylesheet parser, element/class/id/compound selectors,
      cascade, colors, backgrounds, borders, typography, inline styles.
- [x] Milestone 09: Box model, width/height, padding, margin, block layout,
      flex row/column, alignment, gap, min/max, text wrapping, invalidation.

## Phase 4 - Interaction

- [x] Milestone 10: SDL input adapter, mouse and keyboard events, click
      handling, event dispatch, focus management, text input editing.
- [x] Milestone 11: Component abstraction, properties, rendering, lifecycle,
      nesting, reuse.
- [x] Milestone 12: State container, signals, subscriptions, dirty tracking,
      text binding, component updates.

## Phase 5 - Production foundation

- [x] Milestone 13: Button, label, text input, checkbox, image, container,
      scroll container.
- [x] Milestone 14: Image loading, font loading/cache, resource paths, error
      handling.
- [x] Milestone 15: CLI build helper, project templates, markup validation,
      diagnostics, documentation, examples.
- [x] Milestone 16: Unit and integration tests, examples, Linux build,
      installation support, versioning, release documentation.

## Known limitations in 1.1.0

- Flex wrapping is implemented for row direction; column wrapping is not.
- No pseudo-class selectors other than the built-in `hover`/`active`/`focus`
  classes.
- No advanced CSS selectors, animations or transitions.
- Text styling is limited to SDL_ttf capabilities (no per-run rich text beyond
  inline element styles).
- Markup cannot attach event handlers directly; use the C++ API after loading,
  or use the DSL for interactive markup.
- UI operations and rendering run on the main thread. There is no cross-thread
  event queue yet.
- The SDL source build defaults to X11; Wayland/OpenGL/Vulkan options are
  intentionally disabled to keep the dependency footprint small.

## Future work

- Windows and macOS backends.
- Vulkan/OpenGL backends.
- Advanced layout algorithms and animation.
- Theme engine, accessibility, hot reload.
- JSX-like compiler transformation.
- Visual designer and component marketplace.
