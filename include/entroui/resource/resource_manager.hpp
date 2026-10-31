#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "entroui/layout/text_measurer.hpp"

namespace EntroUI {

struct ImageData {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> pixels;

    [[nodiscard]] bool valid() const noexcept {
        return width > 0 && height > 0 && !pixels.empty();
    }
};

class ResourceManager : public ImageSizeProvider {
public:
    explicit ResourceManager(std::filesystem::path baseDirectory = {});

    void setBaseDirectory(std::filesystem::path directory);
    [[nodiscard]] const std::filesystem::path& baseDirectory() const noexcept {
        return baseDirectory_;
    }

    void setAllowAbsolutePaths(bool allow) noexcept { allowAbsolutePaths_ = allow; }

    [[nodiscard]] std::filesystem::path resolve(std::string_view relativePath) const;
    [[nodiscard]] std::string readTextFile(std::string_view relativePath) const;
    [[nodiscard]] std::optional<ImageData> loadImage(std::string_view relativePath) const;
    [[nodiscard]] std::optional<Size> imageSize(std::string_view source) const override;

private:
    std::filesystem::path baseDirectory_;
    bool allowAbsolutePaths_ = false;
};

} // namespace EntroUI
