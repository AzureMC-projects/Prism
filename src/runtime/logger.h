#pragma once
#include <string_view>

namespace prism::detail {
enum class LogLevel { info, warning, error };
void log(LogLevel level, std::string_view message) noexcept;
}
