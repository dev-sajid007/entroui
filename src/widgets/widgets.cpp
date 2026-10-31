#include "entroui/widgets/widgets.hpp"

#include <memory>
#include <string>
#include <utility>

#include "entroui/core/element.hpp"

namespace EntroUI::Widgets {

Element button(std::string text) {
    Element element = EntroUI::button(std::move(text));
    element.addClass("eui-button");
    return element;
}

Element label(std::string text) {
    Element element = EntroUI::label(std::move(text));
    element.addClass("eui-label");
    return element;
}

Element textInput(std::string placeholder, std::string value) {
    Element element = EntroUI::input(std::move(placeholder));
    element.addClass("eui-input");
    if (!value.empty()) {
        element.setValue(std::move(value));
    }
    return element;
}

Element checkbox(std::string labelText, bool checked) {
    Element wrapper = EntroUI::div().className("eui-checkbox");
    wrapper.style("display", "flex").style("align-items", "flex-start");

    auto box = std::make_unique<Element>(EntroUI::div().className("eui-checkbox-box"));
    box->setChecked(checked);
    box->toggleClass("checked", checked);
    box->setFocusable(true);
    Element* boxPointer = box.get();

    Element labelElement = EntroUI::span();
    labelElement.className("eui-checkbox-label");
    labelElement.text(std::move(labelText));
    labelElement.onClick([boxPointer](Event& event) {
        event.stopPropagation();
        const bool next = !boxPointer->checked();
        boxPointer->setChecked(next);
        boxPointer->toggleClass("checked", next);
        Event change(EventType::Change);
        change.target = boxPointer;
        boxPointer->dispatchEvent(change);
    });

    wrapper.child(std::move(box));
    wrapper.child(std::move(labelElement));
    return wrapper;
}

Element image(std::string source) {
    Element element = EntroUI::image(std::move(source));
    element.addClass("eui-image");
    return element;
}

Element container() {
    Element element = EntroUI::div();
    element.addClass("eui-container");
    return element;
}

Element scrollView(Element&& content, float height) {
    Element outer = EntroUI::div();
    outer.className("eui-scroll");
    outer.setScrollable(true);
    outer.style("height", std::to_string(static_cast<int>(height)) + "px");
    Element inner = EntroUI::div();
    inner.className("eui-scroll-content");
    inner.child(std::move(content));
    outer.child(std::move(inner));
    return outer;
}

} // namespace EntroUI::Widgets
