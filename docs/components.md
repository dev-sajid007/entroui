# Components

EntroUI supports two component styles.

## Functional components

Any function returning an `Element` is a reusable component:

```cpp
Element card(std::string title, std::string body) {
    return div()
        .className("card")
        .child(h2().text(std::move(title)))
        .child(p().text(std::move(body)));
}

auto page = div()
    .child(card("First", "Body"))
    .child(card("Second", "Another body"));
```

Functional components compose naturally with the DSL and markup-generated trees.

## Class components

`Component` adds lifecycle hooks, component-local state and scheduled
re-rendering:

```cpp
class Counter : public Component {
public:
    Counter() : Component("Counter") {}

    Element render() override {
        return div()
            .style("padding", "20px")
            .child(h1().bindText(count))
            .child(
                button("Increment").onClick([this](Event&) {
                    count.set(count.get() + 1);
                    requestUpdate();
                }));
    }

    void onMount() override {}
    void onUnmount() override {}

private:
    State<int> count{0};
};
```

Mount it as the application root:

```cpp
App app;
app.mountComponent(std::make_unique<Counter>());
return app.run();
```

### Lifecycle

| Hook | When |
|---|---|
| `render()` | Called to build a new root element |
| `onMount()` | After the rendered tree is attached |
| `onUnmount()` | Before the tree is replaced or the app stops |

`requestUpdate()` sets a dirty flag and invokes the app update callback. On the
next UI iteration the component is unmounted, re-rendered, remounted and the
whole tree receives new styles and layout.

### State and bindings

`State<T>` members survive re-renders because the component object survives.
Bindings created in `render()` are owned by their elements and are cancelled
when the old tree is destroyed. This prevents dangling subscriptions after a
re-render.

For simple text updates, bindings alone do not require a re-render:

```cpp
button("+1").onClick([this](Event&) {
    count.set(count.get() + 1); // bound text updates, layout/paint invalidated
});
```

Use `requestUpdate()` when the tree structure or non-text element properties
need to change.

### Nesting

Components can be nested by returning other components' elements from
`render()`, or by using functional components inside a class component. A
component root can contain any number of child elements and event handlers.

## Component-local state

State is deliberately explicit:

- Create `State<T>` as a component member or local capture.
- Bind text with `bindText`.
- Subscribe with `subscribe` when custom behavior is needed.
- Call `requestUpdate()` when a state change affects structure.

State is not thread-safe. Mutate it on the UI thread.
