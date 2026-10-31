#include "platform/sdl/sdl_platform.hpp"

#include <stdexcept>
#include <string>

#include <SDL3_ttf/SDL_ttf.h>
#include <spdlog/spdlog.h>

namespace EntroUI::detail {

SDLPlatform::SDLPlatform() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL initialization failed: ") + SDL_GetError());
    }
    sdlInitialized_ = true;
    if (!TTF_Init()) {
        SDL_Quit();
        sdlInitialized_ = false;
        throw std::runtime_error(std::string("SDL_ttf initialization failed: ") + SDL_GetError());
    }
    ttfInitialized_ = true;
}

SDLPlatform::~SDLPlatform() {
    if (textInput_ && window_ != nullptr) {
        SDL_StopTextInput(window_);
    }
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (ttfInitialized_) {
        TTF_Quit();
        ttfInitialized_ = false;
    }
    if (sdlInitialized_) {
        SDL_Quit();
        sdlInitialized_ = false;
    }
}

void SDLPlatform::createWindow(const std::string& title, int width, int height) {
    window_ = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        throw std::runtime_error(std::string("Unable to create window: ") + SDL_GetError());
    }
    SDL_ShowWindow(window_);
}

void SDLPlatform::createRenderer() {
    if (window_ == nullptr) {
        throw std::runtime_error("Cannot create a renderer without a window");
    }
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (renderer_ == nullptr) {
        throw std::runtime_error(std::string("Unable to create renderer: ") + SDL_GetError());
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    spdlog::info("SDL renderer: {}", SDL_GetRendererName(renderer_));
}

void SDLPlatform::setTitle(const std::string& title) {
    if (window_ != nullptr) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

void SDLPlatform::windowSize(int& width, int& height) const {
    width = 0;
    height = 0;
    if (window_ != nullptr) {
        SDL_GetWindowSize(window_, &width, &height);
    }
}

void SDLPlatform::startTextInput() {
    if (window_ != nullptr && !textInput_) {
        SDL_StartTextInput(window_);
        textInput_ = true;
    }
}

void SDLPlatform::stopTextInput() {
    if (window_ != nullptr && textInput_) {
        SDL_StopTextInput(window_);
        textInput_ = false;
    }
}

} // namespace EntroUI::detail
