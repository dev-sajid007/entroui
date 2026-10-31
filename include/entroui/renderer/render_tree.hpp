#pragma once

#include "entroui/renderer/render_command.hpp"

namespace EntroUI {

class LayoutTree;
class Element;

[[nodiscard]] RenderCommandList buildRenderCommands(const LayoutTree& layout,
                                                    const Element* focusedElement = nullptr);

} // namespace EntroUI
