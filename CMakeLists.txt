cmake_minimum_required(VERSION 3.25)

project(EntroUI VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(FetchContent)

option(ENTROUI_BUILD_SDL "Build the SDL3 platform and renderer backend" ON)
option(ENTROUI_BUILD_TESTS "Build the automated test suite" ON)
option(ENTROUI_BUILD_EXAMPLES "Build the example applications" ON)
option(ENTROUI_BUILD_CLI "Build the entroui command line tool" ON)
option(ENTROUI_FETCH_SDL "Fetch and build SDL3 and SDL3_ttf from source" ON)
option(ENTROUI_INSTALL "Generate installation rules and export the EntroUI target" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(spdlog CONFIG QUIET)
if(NOT TARGET spdlog::spdlog)
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
    set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog)
endif()

if(ENTROUI_BUILD_SDL)
    if(ENTROUI_FETCH_SDL)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(SDL_INSTALL ON CACHE BOOL "" FORCE)
        set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
        set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
        set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
        set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
        set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
        set(SDL_POWER OFF CACHE BOOL "" FORCE)
        set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
        set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
        set(SDL_GPU OFF CACHE BOOL "" FORCE)
        set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
        set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
        set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
        set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)
        set(SDL_LIBUDEV OFF CACHE BOOL "" FORCE)
        set(SDL_DBUS OFF CACHE BOOL "" FORCE)
        set(SDL_X11 ON CACHE BOOL "" FORCE)
        set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)

        set(SDLTTF_VENDORED OFF CACHE BOOL "" FORCE)
        set(SDLTTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
        set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)
        set(SDLTTF_TESTS OFF CACHE BOOL "" FORCE)
        set(SDLTTF_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
        set(SDLTTF_INSTALL ON CACHE BOOL "" FORCE)
        set(SDLTTF_INSTALL_CPACK OFF CACHE BOOL "" FORCE)
        set(SDLTTF_SHARED OFF CACHE BOOL "" FORCE)
        set(SDLTTF_STATIC ON CACHE BOOL "" FORCE)

        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.16
            GIT_SHALLOW TRUE
        )
        FetchContent_Declare(
            SDL3_ttf
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
            GIT_TAG release-3.2.2
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(SDL3 SDL3_ttf)
    else()
        find_package(SDL3 CONFIG REQUIRED)
        find_package(SDL3_ttf CONFIG REQUIRED)
    endif()
endif()

add_library(entroui
    src/core/node.cpp
    src/core/element.cpp
    src/core/text_node.cpp
    src/core/document.cpp
    src/core/component.cpp
    src/parser/lexer.cpp
    src/parser/parser.cpp
    src/parser/builder.cpp
    src/style/style.cpp
    src/style/selector.cpp
    src/style/stylesheet.cpp
    src/style/computed_style.cpp
    src/layout/box.cpp
    src/layout/layout_engine.cpp
    src/events/event.cpp
    src/events/event_dispatcher.cpp
    src/state/state.cpp
    src/renderer/render_command.cpp
    src/renderer/render_tree.cpp
    src/resource/resource_manager.cpp
    src/widgets/widgets.cpp
    src/app.cpp
)

add_library(EntroUI::entroui ALIAS entroui)

target_include_directories(entroui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(entroui PRIVATE spdlog::spdlog)
target_compile_features(entroui PUBLIC cxx_std_20)

target_compile_definitions(entroui
    PUBLIC
        ENTROUI_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        ENTROUI_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        ENTROUI_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        ENTROUI_VERSION_STRING="${PROJECT_VERSION}"
)

if(ENTROUI_BUILD_SDL)
    target_sources(entroui PRIVATE
        src/platform/sdl/sdl_platform.cpp
        src/renderer/sdl/sdl_font.cpp
        src/renderer/sdl/sdl_renderer.cpp
        src/renderer/sdl/sdl_text_measurer.cpp
    )
    target_compile_definitions(entroui PUBLIC ENTROUI_HAS_SDL=1)
    target_link_libraries(entroui PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
else()
    target_compile_definitions(entroui PUBLIC ENTROUI_HAS_SDL=0)
endif()

if(MSVC)
    target_compile_options(entroui PRIVATE /W4 /permissive-)
else()
    target_compile_options(entroui PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ENTROUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ENTROUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ENTROUI_BUILD_CLI)
    add_executable(entroui-cli tools/entroui_cli.cpp)
    set_target_properties(entroui-cli PROPERTIES OUTPUT_NAME entroui)
    target_link_libraries(entroui-cli PRIVATE entroui)
    target_compile_definitions(entroui-cli PRIVATE
        ENTROUI_TEMPLATE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

if(ENTROUI_INSTALL)
    include(CMakePackageConfigHelpers)

    install(TARGETS entroui
        EXPORT EntroUITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/entroui DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT EntroUITargets
        FILE EntroUITargets.cmake
        NAMESPACE EntroUI::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EntroUI
    )

    configure_package_config_file(
        cmake/EntroUIConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/EntroUIConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EntroUI
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/EntroUIConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/EntroUIConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/EntroUIConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EntroUI
    )

    install(DIRECTORY ui DESTINATION ${CMAKE_INSTALL_DATADIR}/entroui OPTIONAL)
endif()

message(STATUS "EntroUI ${PROJECT_VERSION}: SDL=${ENTROUI_BUILD_SDL} tests=${ENTROUI_BUILD_TESTS} examples=${ENTROUI_BUILD_EXAMPLES}")
