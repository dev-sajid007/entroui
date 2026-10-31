#include "renderer/sdl/sdl_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>

namespace EntroUI::detail {

namespace {

SDL_FRect toFRect(const Rect& rect) {
    return SDL_FRect{rect.x, rect.y, std::max(0.0F, rect.width), std::max(0.0F, rect.height)};
}

SDL_Rect toClipRect(const Rect& rect) {
    const int left = static_cast<int>(std::floor(rect.x));
    const int top = static_cast<int>(std::floor(rect.y));
    const int right = static_cast<int>(std::ceil(rect.right()));
    const int bottom = static_cast<int>(std::ceil(rect.bottom()));
    return SDL_Rect{left, top, std::max(0, right - left), std::max(0, bottom - top)};
}

SDL_Rect intersectRect(const SDL_Rect& a, const SDL_Rect& b) {
    const int left = std::max(a.x, b.x);
    const int top = std::max(a.y, b.y);
    const int right = std::min(a.x + a.w, b.x + b.w);
    const int bottom = std::min(a.y + a.h, b.y + b.h);
    if (right <= left || bottom <= top) {
        return SDL_Rect{0, 0, 0, 0};
    }
    return SDL_Rect{left, top, right - left, bottom - top};
}

std::string textureKey(TTF_Font* font, const std::string& text, const Color& color) {
    return std::to_string(reinterpret_cast<std::uintptr_t>(font)) + "|" +
           std::to_string(color.rgba()) + "|" + text;
}

} // namespace

SDLRenderer::SDLRenderer(SDL_Renderer* renderer, FontCache& fonts, ResourceManager& resources)
    : renderer_(renderer), fonts_(fonts), resources_(resources) {}

SDLRenderer::~SDLRenderer() { clearTextures(); }

void SDLRenderer::clearTextures() {
    for (auto& [key, texture] : textTextures_) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
    }
    textTextures_.clear();
    for (auto& [key, texture] : imageTextures_) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
    }
    imageTextures_.clear();
}

void SDLRenderer::setColor(const Color& color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
}

void SDLRenderer::beginFrame(int width, int height, const Color& clearColor) {
    frameWidth_ = width;
    frameHeight_ = height;
    clipStack_.clear();
    SDL_SetRenderClipRect(renderer_, nullptr);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    setColor(clearColor);
    SDL_RenderClear(renderer_);
}

void SDLRenderer::drawRoundedFilled(const SDL_FRect& rect, float radius) {
    const int width = static_cast<int>(std::lround(rect.w));
    const int height = static_cast<int>(std::lround(rect.h));
    if (width <= 0 || height <= 0) {
        return;
    }
    const int maxRadius = std::min(width, height) / 2;
    const int r = std::clamp(static_cast<int>(std::lround(radius)), 0, maxRadius);
    if (r <= 0) {
        SDL_RenderFillRect(renderer_, &rect);
        return;
    }

    const float left = rect.x;
    const float top = rect.y;
    for (int y = 0; y < height; ++y) {
        float inset = 0.0F;
        if (y < r) {
            const float dy = static_cast<float>(r - y) - 0.5F;
            inset = static_cast<float>(r) - std::sqrt(std::max(0.0F, static_cast<float>(r * r) -
                                                                            dy * dy));
        } else if (y >= height - r) {
            const float dy = static_cast<float>(y - (height - r)) + 0.5F;
            inset = static_cast<float>(r) - std::sqrt(std::max(0.0F, static_cast<float>(r * r) -
                                                                            dy * dy));
        }
        SDL_FRect row{left + inset, top + static_cast<float>(y),
                      static_cast<float>(width) - inset * 2.0F, 1.0F};
        if (row.w > 0.0F) {
            SDL_RenderFillRect(renderer_, &row);
        }
    }
}

void SDLRenderer::drawRect(const Rect& rect, const Color& color, float radius) {
    if (!color.visible() || rect.width <= 0.0F || rect.height <= 0.0F) {
        return;
    }
    setColor(color);
    drawRoundedFilled(toFRect(rect), radius);
}

void SDLRenderer::drawRoundedOutline(const SDL_FRect& rect, float width, float radius) {
    const float r = std::clamp(radius, 0.0F, std::min(rect.w, rect.h) * 0.5F);
    const float thickness = std::max(1.0F, width);
    if (r <= 0.0F) {
        SDL_FRect top{rect.x, rect.y, rect.w, thickness};
        SDL_FRect bottom{rect.x, rect.y + rect.h - thickness, rect.w, thickness};
        SDL_FRect left{rect.x, rect.y, thickness, rect.h};
        SDL_FRect right{rect.x + rect.w - thickness, rect.y, thickness, rect.h};
        SDL_RenderFillRect(renderer_, &top);
        SDL_RenderFillRect(renderer_, &bottom);
        SDL_RenderFillRect(renderer_, &left);
        SDL_RenderFillRect(renderer_, &right);
        return;
    }

    SDL_FRect top{rect.x + r, rect.y, std::max(0.0F, rect.w - 2.0F * r), thickness};
    SDL_FRect bottom{rect.x + r, rect.y + rect.h - thickness,
                     std::max(0.0F, rect.w - 2.0F * r), thickness};
    SDL_FRect left{rect.x, rect.y + r, thickness, std::max(0.0F, rect.h - 2.0F * r)};
    SDL_FRect right{rect.x + rect.w - thickness, rect.y + r, thickness,
                    std::max(0.0F, rect.h - 2.0F * r)};
    SDL_RenderFillRect(renderer_, &top);
    SDL_RenderFillRect(renderer_, &bottom);
    SDL_RenderFillRect(renderer_, &left);
    SDL_RenderFillRect(renderer_, &right);

    constexpr int segments = 8;
    const float pi = 3.14159265358979323846F;
    struct Corner {
        float cx;
        float cy;
        float start;
    };
    const Corner corners[4] = {
        {rect.x + r, rect.y + r, pi},
        {rect.x + rect.w - r, rect.y + r, pi * 1.5F},
        {rect.x + rect.w - r, rect.y + rect.h - r, 0.0F},
        {rect.x + r, rect.y + rect.h - r, pi * 0.5F},
    };
    for (const auto& corner : corners) {
        for (int index = 0; index < segments; ++index) {
            const float a0 =
                corner.start + (pi * 0.5F) * static_cast<float>(index) / segments;
            const float a1 =
                corner.start + (pi * 0.5F) * static_cast<float>(index + 1) / segments;
            SDL_RenderLine(renderer_, corner.cx + std::cos(a0) * r,
                           corner.cy + std::sin(a0) * r, corner.cx + std::cos(a1) * r,
                           corner.cy + std::sin(a1) * r);
        }
    }
}

void SDLRenderer::drawBorder(const Rect& rect, const Color& color, float width, float radius) {
    if (!color.visible() || width <= 0.0F || rect.width <= 0.0F || rect.height <= 0.0F) {
        return;
    }
    setColor(color);
    drawRoundedOutline(toFRect(rect), width, radius);
}

SDL_Texture* SDLRenderer::getTextTexture(TTF_Font* font, const std::string& text,
                                         const Color& color) {
    const std::string key = textureKey(font, text, color);
    const auto it = textTextures_.find(key);
    if (it != textTextures_.end()) {
        return it->second;
    }
    if (textTextures_.size() > 512) {
        for (auto& [oldKey, texture] : textTextures_) {
            if (texture != nullptr) {
                SDL_DestroyTexture(texture);
            }
        }
        textTextures_.clear();
    }

    const SDL_Color foreground{color.r, color.g, color.b, color.a};
    SDL_Surface* surface =
        TTF_RenderText_Blended(font, text.c_str(), text.size(), foreground);
    if (surface == nullptr) {
        textTextures_.emplace(key, nullptr);
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);
    if (texture != nullptr) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(texture, color.a);
    }
    textTextures_.emplace(key, texture);
    return texture;
}

void SDLRenderer::drawText(const TextCommand& command) {
    if (command.text.empty() || command.rect.height <= 0.0F) {
        return;
    }
    TTF_Font* font = fonts_.get(command.font);
    if (font == nullptr) {
        return;
    }
    SDL_Texture* texture = getTextTexture(font, command.text, command.color);
    if (texture == nullptr) {
        return;
    }
    float textWidth = 0.0F;
    float textHeight = 0.0F;
    if (!SDL_GetTextureSize(texture, &textWidth, &textHeight)) {
        return;
    }

    float x = command.rect.x;
    if (command.align == TextAlign::Center) {
        x += (command.rect.width - textWidth) * 0.5F;
    } else if (command.align == TextAlign::Right) {
        x += command.rect.width - textWidth;
    }
    const float y = command.rect.y +
                    std::max(0.0F, (command.rect.height - textHeight) * 0.5F);
    SDL_FRect destination{x, y, textWidth, textHeight};
    SDL_RenderTexture(renderer_, texture, nullptr, &destination);
}

SDL_Texture* SDLRenderer::getImageTexture(const std::string& source) {
    const auto it = imageTextures_.find(source);
    if (it != imageTextures_.end()) {
        return it->second;
    }
    SDL_Texture* texture = nullptr;
    try {
        if (auto image = resources_.loadImage(source); image && image->valid()) {
            texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                        SDL_TEXTUREACCESS_STATIC, image->width, image->height);
            if (texture != nullptr) {
                SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
                if (!SDL_UpdateTexture(texture, nullptr, image->pixels.data(),
                                       image->width * 4)) {
                    SDL_DestroyTexture(texture);
                    texture = nullptr;
                }
            }
        }
    } catch (const std::exception& exception) {
        spdlog::warn("Unable to load image '{}': {}", source, exception.what());
    }
    imageTextures_.emplace(source, texture);
    return texture;
}

void SDLRenderer::drawImage(const ImageCommand& command) {
    if (command.rect.width <= 0.0F || command.rect.height <= 0.0F) {
        return;
    }
    SDL_Texture* texture = getImageTexture(command.source);
    if (texture == nullptr) {
        return;
    }
    const SDL_FRect destination = toFRect(command.rect);
    SDL_RenderTexture(renderer_, texture, nullptr, &destination);
}

void SDLRenderer::pushClip(const Rect& rect) {
    SDL_Rect clip = toClipRect(rect);
    if (!clipStack_.empty()) {
        clip = intersectRect(clipStack_.back(), clip);
    }
    clipStack_.push_back(clip);
    SDL_SetRenderClipRect(renderer_, &clipStack_.back());
}

void SDLRenderer::popClip() {
    if (clipStack_.empty()) {
        return;
    }
    clipStack_.pop_back();
    if (clipStack_.empty()) {
        SDL_SetRenderClipRect(renderer_, nullptr);
    } else {
        SDL_SetRenderClipRect(renderer_, &clipStack_.back());
    }
}

void SDLRenderer::endFrame() { SDL_RenderPresent(renderer_); }

} // namespace EntroUI::detail
