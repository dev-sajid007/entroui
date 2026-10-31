#include "renderer/sdl/sdl_font.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

namespace EntroUI::detail {

namespace {

const std::vector<std::string>& regularCandidates() {
    static const std::vector<std::string> candidates = {
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/google-noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/cantarell/Cantarell-Regular.otf",
        "/usr/share/fonts/abattis-cantarell-fonts/Cantarell-Regular.otf",
    };
    return candidates;
}

const std::vector<std::string>& boldCandidates() {
    static const std::vector<std::string> candidates = {
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/google-noto/NotoSans-Bold.ttf",
        "/usr/share/fonts/noto/NotoSans-Bold.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
        "/usr/share/fonts/cantarell/Cantarell-Bold.otf",
        "/usr/share/fonts/abattis-cantarell-fonts/Cantarell-Bold.otf",
    };
    return candidates;
}

} // namespace

FontCache::FontCache() {
    regularPath_ = findSystemFont(false);
    boldPath_ = findSystemFont(true);
    if (regularPath_.empty()) {
        spdlog::warn("No system font found; text rendering will use approximate metrics");
    }
}

FontCache::~FontCache() {
    for (auto& [key, font] : fonts_) {
        if (font != nullptr) {
            TTF_CloseFont(font);
        }
    }
    fonts_.clear();
}

std::string FontCache::findSystemFont(bool bold) {
    const auto& candidates = bold ? boldCandidates() : regularCandidates();
    for (const auto& candidate : candidates) {
        std::error_code error;
        if (std::filesystem::exists(candidate, error) && !error) {
            return candidate;
        }
    }
    if (bold) {
        return findSystemFont(false);
    }
    return {};
}

void FontCache::setDefaultFont(const std::string& regularPath, const std::string& boldPath) {
    for (auto& [key, font] : fonts_) {
        if (font != nullptr) {
            TTF_CloseFont(font);
        }
    }
    fonts_.clear();
    regularPath_ = regularPath.empty() ? findSystemFont(false) : regularPath;
    boldPath_ = boldPath.empty() ? findSystemFont(true) : boldPath;
}

TTF_Font* FontCache::get(const FontSpec& fontSpec) {
    std::string path;
    if (!fontSpec.family.empty()) {
        std::error_code error;
        if (std::filesystem::exists(fontSpec.family, error) && !error) {
            path = fontSpec.family;
        }
    }
    const bool bold = fontSpec.weight >= 600;
    if (path.empty()) {
        path = bold && !boldPath_.empty() ? boldPath_ : regularPath_;
    }
    if (path.empty()) {
        return nullptr;
    }

    const int pointSize =
        std::clamp(static_cast<int>(std::lround(fontSpec.size)), 1, 512);
    const std::string key = path + "#" + std::to_string(pointSize) + "#" +
                            (bold ? "bold" : "regular");
    const auto existing = fonts_.find(key);
    if (existing != fonts_.end()) {
        return existing->second;
    }

    TTF_Font* font = TTF_OpenFont(path.c_str(), static_cast<float>(pointSize));
    if (font == nullptr) {
        spdlog::warn("Unable to open font '{}': {}", path, SDL_GetError());
        return nullptr;
    }
    if (bold) {
        TTF_SetFontStyle(font, TTF_STYLE_BOLD);
    }
    fonts_.emplace(key, font);
    return font;
}

} // namespace EntroUI::detail
