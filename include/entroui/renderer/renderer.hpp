#pragma once

#include "entroui/layout/box.hpp"
#include "entroui/renderer/render_command.hpp"
#include "entroui/style/style.hpp"

namespace EntroUI {

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void beginFrame(int width, int height, const Color& clearColor) = 0;
    virtual void drawRect(const Rect& rect, const Color& color, float radius) = 0;
    virtual void drawBorder(const Rect& rect, const Color& color, float width, float radius) = 0;
    virtual void drawText(const TextCommand& command) = 0;
    virtual void drawImage(const ImageCommand& command) = 0;
    virtual void pushClip(const Rect& rect) = 0;
    virtual void popClip() = 0;
    virtual void endFrame() = 0;
};

void executeRenderCommands(Renderer& renderer, const RenderCommandList& commands);

} // namespace EntroUI
