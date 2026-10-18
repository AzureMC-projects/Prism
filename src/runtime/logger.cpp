#include "logger.h"
#include <Windows.h>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace prism::detail {
namespace {
std::mutex g_mutex;
std::filesystem::path log_path() {
  wchar_t local[MAX_PATH]{};
  const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return std::filesystem::current_path() / L"Prism.log";
  return std::filesystem::path(local) / L"Prism" / L"logs" / L"Prism.log";
}
const char* label(LogLevel level) {
  switch (level) {
    case LogLevel::warning: return "WARN";
    case LogLevel::error: return "ERROR";
    default: return "INFO";
  }
}
}
void log(LogLevel level, std::string_view message) noexcept {
  try {
    std::lock_guard lock(g_mutex);
    const auto path = log_path();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::app);
    if (!out) return;
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm local{};
    localtime_s(&local, &now);
    char stamp[32]{};
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &local);
    out << '[' << stamp << "] [" << label(level) << "] " << message << '\n';
  } catch (...) {
    // Diagnostics must never crash the host process.
  }
}
}
