#include "entroui/config.hpp"
#include "entroui/parser/builder.hpp"
#include "entroui/parser/parser.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifndef ENTROUI_TEMPLATE_ROOT
#define ENTROUI_TEMPLATE_ROOT ""
#endif

using namespace EntroUI;

namespace {

namespace fs = std::filesystem;

const char* kUsage = R"(EntroUI command line tool

Usage:
  entroui create <directory>     Create a new EntroUI application
  entroui build [directory]      Configure and build the application
  entroui run [directory]        Run the built application
  entroui test [directory]       Run application tests with CTest
  entroui check [path...]        Validate .eui markup files
  entroui version                Print the EntroUI version
  entroui help                   Show this help
)";

std::string shellQuote(std::string_view value) {
    std::string result = "'";
    for (char character : value) {
        if (character == '\'') {
            result += "'\\''";
        } else {
            result += character;
        }
    }
    result += '\'';
    return result;
}

bool isValidProjectName(std::string_view name) {
    if (name.empty()) {
        return false;
    }
    return std::all_of(name.begin(), name.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' || character == '_';
    });
}

bool writeFile(const fs::path& path, std::string_view contents) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    stream << contents;
    return static_cast<bool>(stream);
}

int runShell(const std::string& command, const fs::path& workingDirectory = {}) {
    std::string fullCommand = command;
    if (!workingDirectory.empty()) {
        fullCommand = "cd " + shellQuote(workingDirectory.string()) + " && " + command;
    }
    return std::system(fullCommand.c_str());
}

void enableLocalEntroUI() {
    if (*ENTROUI_TEMPLATE_ROOT == '\0') {
        return;
    }
    std::error_code error;
    if (fs::exists(fs::path(ENTROUI_TEMPLATE_ROOT) / "CMakeLists.txt", error) && !error) {
#ifdef _WIN32
        _putenv_s("ENTROUI_ROOT", ENTROUI_TEMPLATE_ROOT);
#else
        setenv("ENTROUI_ROOT", ENTROUI_TEMPLATE_ROOT, 1);
#endif
    }
}

const char* kCMakeTemplate = R"CMAKE(cmake_minimum_required(VERSION 3.25)

project(@APP_NAME@ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENTROUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENTROUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENTROUI_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(ENTROUI_INSTALL OFF CACHE BOOL "" FORCE)

if(DEFINED ENV{ENTROUI_ROOT})
    add_subdirectory("$ENV{ENTROUI_ROOT}" entroui)
else()
    find_package(EntroUI CONFIG REQUIRED)
endif()

add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE EntroUI::entroui)
target_compile_definitions(app PRIVATE APP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
)CMAKE";

const char* kMainTemplate = R"CPP(#include <entroui/entroui.hpp>

#include <filesystem>

using namespace EntroUI;

int main() {
    const std::filesystem::path sourceDirectory = APP_SOURCE_DIR;

    App app;
    app.title("@APP_NAME@")
        .size(900, 600)
        .background("#f8fafc")
        .load(sourceDirectory / "ui" / "app.eui")
        .stylesheet(sourceDirectory / "ui" / "styles.css");

    return app.run();
}
)CPP";

const char* kMarkupTemplate = R"EUI(<app>
    <window title="@APP_NAME@" width="900" height="600">
        <div class="container">
            <h1>@APP_NAME@</h1>
            <p>Your new EntroUI application is ready.</p>
            <button id="hello">Say hello</button>
        </div>
    </window>
</app>
)EUI";

const char* kStylesTemplate = R"CSS(.container {
    padding: 32px;
}

h1 {
    font-size: 36px;
    color: #0f172a;
    margin-bottom: 12px;
}

p {
    color: #475569;
    line-height: 1.6;
    margin-bottom: 20px;
}

button {
    background: #2563eb;
    color: #ffffff;
    padding: 10px 18px;
    border-radius: 8px;
}
)CSS";

const char* kConanTemplate = R"PYTHON(from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class Application(ConanFile):
    name = "@APP_NAME@"
    version = "0.1.0"
    settings = "os", "arch", "compiler", "build_type"
    requires = "spdlog/1.17.0"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        CMakeToolchain(self).generate()
        CMakeDeps(self).generate()
)PYTHON";

const char* kReadmeTemplate = R"MD(# @APP_NAME@

An EntroUI desktop application.

## Build and run

```bash
entroui build
entroui run
```

Or manually:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/app
```

The application UI lives in `ui/app.eui` and `ui/styles.css`.
)MD";

int createProject(const fs::path& directory, const std::string& name) {
    std::error_code error;
    if (fs::exists(directory, error) && !error &&
        fs::is_directory(directory, error) && !fs::is_empty(directory, error)) {
        std::cerr << "error: directory already exists and is not empty: " << directory << '\n';
        return 1;
    }
    if (!fs::create_directories(directory / "src", error) && error) {
        std::cerr << "error: unable to create project directory: " << error.message() << '\n';
        return 1;
    }
    fs::create_directories(directory / "ui", error);
    fs::create_directories(directory / "assets", error);

    auto replaceName = [&name](const char* text) {
        std::string value(text);
        std::size_t position = 0;
        while ((position = value.find("@APP_NAME@", position)) != std::string::npos) {
            value.replace(position, 10, name);
            position += name.size();
        }
        return value;
    };

    const bool ok = writeFile(directory / "CMakeLists.txt", replaceName(kCMakeTemplate)) &&
                    writeFile(directory / "src" / "main.cpp", replaceName(kMainTemplate)) &&
                    writeFile(directory / "ui" / "app.eui", replaceName(kMarkupTemplate)) &&
                    writeFile(directory / "ui" / "styles.css", kStylesTemplate) &&
                    writeFile(directory / "conanfile.py", replaceName(kConanTemplate)) &&
                    writeFile(directory / "README.md", replaceName(kReadmeTemplate)) &&
                    writeFile(directory / "assets" / ".gitkeep", "") &&
                    writeFile(directory / ".gitignore", "build/\n");
    if (!ok) {
        std::cerr << "error: failed to write project files\n";
        return 1;
    }

    std::cout << "Created EntroUI application '" << name << "' in " << directory << "\n";
    std::cout << "Next: entroui build " << directory << " && entroui run " << directory
              << '\n';
    return 0;
}

fs::path projectDirectory(const std::string& argument) {
    return argument.empty() ? fs::current_path() : fs::path(argument);
}

int buildProject(const fs::path& directory) {
    enableLocalEntroUI();
    if (!fs::exists(directory / "CMakeLists.txt")) {
        std::cerr << "error: no CMakeLists.txt found in " << directory << '\n';
        return 1;
    }
    const auto buildDirectory = directory / "build";
    const std::string configure = "cmake -S " + shellQuote(directory.string()) + " -B " +
                                  shellQuote(buildDirectory.string()) +
                                  " -G Ninja -DCMAKE_BUILD_TYPE=Debug";
    if (runShell(configure) != 0) {
        std::cerr << "error: CMake configuration failed\n";
        return 1;
    }
    const unsigned int jobs = std::max(1U, std::thread::hardware_concurrency());
    const std::string build = "cmake --build " + shellQuote(buildDirectory.string()) +
                              " -j " + std::to_string(jobs);
    if (runShell(build) != 0) {
        std::cerr << "error: build failed\n";
        return 1;
    }
    std::cout << "Build complete: " << (buildDirectory / "app") << '\n';
    return 0;
}

int runProject(const fs::path& directory) {
    std::error_code error;
    const fs::path executable = fs::absolute(directory / "build" / "app", error);
    if (!fs::exists(executable)) {
        std::cerr << "error: application is not built; run 'entroui build "
                  << directory.string() << "' first\n";
        return 1;
    }
    return runShell(shellQuote(executable.string()), directory);
}

int testProject(const fs::path& directory) {
    const auto buildDirectory = directory / "build";
    if (!fs::exists(buildDirectory / "CMakeCache.txt")) {
        std::cerr << "error: project is not configured; run 'entroui build "
                  << directory.string() << "' first\n";
        return 1;
    }
    return runShell("ctest --output-on-failure", buildDirectory);
}

int checkFile(const fs::path& path) {
    Parser parser;
    auto parsed = parser.parseFile(path);
    int errors = 0;
    for (const auto& diagnostic : parsed.diagnostics) {
        std::cerr << diagnostic.format() << '\n';
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            ++errors;
        }
    }
    if (parsed.ok() && errors == 0) {
        auto built = buildDocument(*parsed.value);
        for (const auto& diagnostic : built.diagnostics) {
            std::cerr << diagnostic.format() << '\n';
            if (diagnostic.severity == DiagnosticSeverity::Error) {
                ++errors;
            }
        }
    }
    if (errors == 0) {
        std::cout << "OK " << path.string() << '\n';
    }
    return errors;
}

int checkPaths(const std::vector<std::string>& arguments) {
    std::vector<fs::path> files;
    for (const auto& argument : arguments) {
        const fs::path path(argument);
        if (fs::is_directory(path)) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".eui") {
                    files.push_back(entry.path());
                }
            }
        } else {
            files.push_back(path);
        }
    }
    if (files.empty()) {
        if (fs::is_directory("ui")) {
            for (const auto& entry : fs::recursive_directory_iterator("ui")) {
                if (entry.is_regular_file() && entry.path().extension() == ".eui") {
                    files.push_back(entry.path());
                }
            }
        }
    }
    if (files.empty()) {
        std::cerr << "error: no .eui files found\n";
        return 1;
    }

    int errors = 0;
    for (const auto& file : files) {
        errors += checkFile(file);
    }
    return errors == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << kUsage;
        return 0;
    }

    const std::string command = argv[1];
    if (command == "help" || command == "--help" || command == "-h") {
        std::cout << kUsage;
        return 0;
    }
    if (command == "version" || command == "--version") {
        std::cout << "EntroUI " << EntroUI::kVersion << '\n';
        return 0;
    }
    if (command == "create") {
        if (argc < 3) {
            std::cerr << "error: create requires a directory name\n";
            return 2;
        }
        const fs::path directory = argv[2];
        const std::string name = directory.filename().string();
        if (!isValidProjectName(name)) {
            std::cerr << "error: project name may only contain letters, digits, '-' and '_'\n";
            return 2;
        }
        return createProject(directory, name);
    }
    if (command == "build") {
        return buildProject(projectDirectory(argc >= 3 ? argv[2] : ""));
    }
    if (command == "run") {
        return runProject(projectDirectory(argc >= 3 ? argv[2] : ""));
    }
    if (command == "test") {
        return testProject(projectDirectory(argc >= 3 ? argv[2] : ""));
    }
    if (command == "check") {
        std::vector<std::string> paths;
        for (int index = 2; index < argc; ++index) {
            paths.emplace_back(argv[index]);
        }
        return checkPaths(paths);
    }

    std::cerr << "error: unknown command '" << command << "'\n\n" << kUsage;
    return 2;
}
