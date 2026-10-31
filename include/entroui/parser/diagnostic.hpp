#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace EntroUI {

enum class DiagnosticSeverity { Error, Warning };

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string message;
    std::string file;
    std::size_t line = 1;
    std::size_t column = 1;

    [[nodiscard]] std::string format() const;
};

template <typename T>
struct ParseResult {
    std::optional<T> value;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool ok() const noexcept { return value.has_value(); }
    explicit operator bool() const noexcept { return ok(); }

    [[nodiscard]] T& operator*() { return *value; }
    [[nodiscard]] const T& operator*() const { return *value; }
    [[nodiscard]] T* operator->() { return &*value; }
    [[nodiscard]] const T* operator->() const { return &*value; }
};

inline Diagnostic makeError(std::string message, std::string file, std::size_t line,
                            std::size_t column) {
    return Diagnostic{DiagnosticSeverity::Error, std::move(message), std::move(file), line, column};
}

inline Diagnostic makeWarning(std::string message, std::string file, std::size_t line,
                              std::size_t column) {
    return Diagnostic{DiagnosticSeverity::Warning, std::move(message), std::move(file), line,
                      column};
}

} // namespace EntroUI
