#include "entroui/resource/resource_manager.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

namespace EntroUI {

ResourceManager::ResourceManager(std::filesystem::path baseDirectory) {
    setBaseDirectory(baseDirectory.empty() ? std::filesystem::current_path()
                                           : std::move(baseDirectory));
}

void ResourceManager::setBaseDirectory(std::filesystem::path directory) {
    std::error_code error;
    auto absolute = std::filesystem::absolute(directory, error);
    if (error) {
        absolute = directory;
    }
    auto canonical = std::filesystem::weakly_canonical(absolute, error);
    baseDirectory_ = error ? absolute : canonical;
}

std::filesystem::path ResourceManager::resolve(std::string_view relativePath) const {
    if (relativePath.empty()) {
        throw std::runtime_error("Resource path is empty");
    }
    std::filesystem::path requested(relativePath);
    if (requested.is_absolute() && !allowAbsolutePaths_) {
        throw std::runtime_error("Absolute resource paths are not allowed: " +
                                 std::string(relativePath));
    }

    std::error_code error;
    const auto base = std::filesystem::weakly_canonical(baseDirectory_, error);
    const auto target = std::filesystem::weakly_canonical(
        requested.is_absolute() ? requested : base / requested, error);
    if (error) {
        throw std::runtime_error("Unable to resolve resource path: " +
                                 std::string(relativePath));
    }
    if (!allowAbsolutePaths_) {
        const auto relative = target.lexically_relative(base);
        if (relative.empty() || (!relative.empty() && *relative.begin() == "..")) {
            throw std::runtime_error("Resource path escapes the resource directory: " +
                                     std::string(relativePath));
        }
    }
    return target;
}

std::string ResourceManager::readTextFile(std::string_view relativePath) const {
    const auto path = resolve(relativePath);
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open file: " + path.string());
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::optional<ImageData> ResourceManager::loadImage(std::string_view relativePath) const {
    std::filesystem::path path;
    try {
        path = resolve(relativePath);
    } catch (const std::exception& exception) {
        spdlog::warn("Resource load failed: {}", exception.what());
        return std::nullopt;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        spdlog::warn("Unable to load image '{}': {}", path.string(), stbi_failure_reason());
        return std::nullopt;
    }

    ImageData data;
    data.width = width;
    data.height = height;
    data.channels = 4;
    const std::size_t size =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    data.pixels.assign(pixels, pixels + size);
    stbi_image_free(pixels);
    return data;
}

std::optional<Size> ResourceManager::imageSize(std::string_view source) const {
    std::filesystem::path path;
    try {
        path = resolve(source);
    } catch (const std::exception&) {
        return std::nullopt;
    }
    int width = 0;
    int height = 0;
    int channels = 0;
    if (stbi_info(path.string().c_str(), &width, &height, &channels) == 0) {
        return std::nullopt;
    }
    return Size{static_cast<float>(width), static_cast<float>(height)};
}

} // namespace EntroUI
