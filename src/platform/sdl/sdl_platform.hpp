#pragma once

#include <string>

#include <SDL3/SDL.h>

namespace EntroUI::detail {

class SDLPlatform {
public:
    SDLPlatform();
    ~SDLPlatform();

    SDLPlatform(const SDLPlatform&) = delete;
    SDLPlatform& operator=(const SDLPlatform&) = delete;

    void createWindow(const std::string& title, int width, int height);
    void createRenderer();
    void setTitle(const std::string& title);
    void windowSize(int& width, int& height) const;
    void startTextInput();
    void stopTextInput();

    [[nodiscard]] SDL_Window* window() const noexcept { return window_; }
    [[nodiscard]] SDL_Renderer* renderer() const noexcept { return renderer_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool sdlInitialized_ = false;
    bool ttfInitialized_ = false;
    bool textInput_ = false;
};

} // namespace EntroUI::detail
