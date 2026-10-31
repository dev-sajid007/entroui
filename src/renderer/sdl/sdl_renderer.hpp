#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

#include "entroui/renderer/renderer.hpp"
#include "entroui/resource/resource_manager.hpp"
#include "renderer/sdl/sdl_font.hpp"

namespace EntroUI::detail {

class SDLRenderer : public Renderer {
public:
    SDLRenderer(SDL_Renderer* renderer, FontCache& fonts, ResourceManager& resources);
    ~SDLRenderer() override;

    SDLRenderer(const SDLRenderer&) = delete;
    SDLRenderer& operator=(const SDLRenderer&) = delete;

    void beginFrame(int width, int height, const Color& clearColor) override;
    void drawRect(const Rect& rect, const Color& color, float radius) override;
    void drawBorder(const Rect& rect, const Color& color, float width, float radius) override;
    void drawText(const TextCommand& command) override;
    void drawImage(const ImageCommand& command) override;
    void pushClip(const Rect& rect) override;
    void popClip() override;
    void endFrame() override;

private:
    void setColor(const Color& color);
    void drawRoundedFilled(const SDL_FRect& rect, float radius);
    void drawRoundedOutline(const SDL_FRect& rect, float width, float radius);
    [[nodiscard]] SDL_Texture* getTextTexture(TTF_Font* font, const std::string& text,
                                              const Color& color);
    [[nodiscard]] SDL_Texture* getImageTexture(const std::string& source);
    void clearTextures();

    SDL_Renderer* renderer_ = nullptr;
    FontCache& fonts_;
    ResourceManager& resources_;
    std::unordered_map<std::string, SDL_Texture*> textTextures_;
    std::unordered_map<std::string, SDL_Texture*> imageTextures_;
    std::vector<SDL_Rect> clipStack_;
    int frameWidth_ = 0;
    int frameHeight_ = 0;
};

} // namespace EntroUI::detail
