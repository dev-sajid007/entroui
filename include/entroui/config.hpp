#pragma once

#ifndef ENTROUI_VERSION_MAJOR
#define ENTROUI_VERSION_MAJOR 1
#endif
#ifndef ENTROUI_VERSION_MINOR
#define ENTROUI_VERSION_MINOR 0
#endif
#ifndef ENTROUI_VERSION_PATCH
#define ENTROUI_VERSION_PATCH 0
#endif
#ifndef ENTROUI_VERSION_STRING
#define ENTROUI_VERSION_STRING "1.0.0"
#endif
#ifndef ENTROUI_HAS_SDL
#define ENTROUI_HAS_SDL 1
#endif

namespace EntroUI {

inline constexpr const char* kVersion = ENTROUI_VERSION_STRING;
inline constexpr int kVersionMajor = ENTROUI_VERSION_MAJOR;
inline constexpr int kVersionMinor = ENTROUI_VERSION_MINOR;
inline constexpr int kVersionPatch = ENTROUI_VERSION_PATCH;

} // namespace EntroUI
