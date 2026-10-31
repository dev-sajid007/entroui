#pragma once

#include <string>
#include <unordered_map>

#include <SDL3_ttf/SDL_ttf.h>

#include "entroui/style/style.hpp"

namespace EntroUI::detail {

class FontCache {
public:
    FontCache();
    ~FontCache();

    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;

    void setDefaultFont(const std::string& regularPath, const std::string& boldPath = {});
    [[nodiscard]] TTF_Font* get(const FontSpec& font);
    [[nodiscard]] const std::string& regularPath() const noexcept { return regularPath_; }
    [[nodiscard]] const std::string& boldPath() const noexcept { return boldPath_; }

    [[nodiscard]] static std::string findSystemFont(bool bold);

private:
    std::string regularPath_;
    std::string boldPath_;
    std::unordered_map<std::string, TTF_Font*> fonts_;
};

} // namespace EntroUI::detail
