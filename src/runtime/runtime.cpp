#include <Windows.h>
#include <filesystem>
#include <memory>
#include "logger.h"
#include "plugin_manager.h"

namespace {
std::unique_ptr<prism::detail::PluginManager> g_manager;
std::filesystem::path install_root() {
  HMODULE self{};
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCWSTR>(&install_root), &self)) {
    return std::filesystem::current_path() / L"Prism";
  }
  wchar_t path[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(self, path, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return std::filesystem::current_path() / L"Prism";
  return std::filesystem::path(path).parent_path();
}
}

extern "C" __declspec(dllexport) bool __cdecl PrismInitialize() noexcept {
  try {
    if (g_manager) return true;
    prism::detail::log(prism::detail::LogLevel::info, "Prism runtime initialization started.");
    g_manager = std::make_unique<prism::detail::PluginManager>();
    g_manager->load_directory(install_root() / L"mods");
    prism::detail::log(prism::detail::LogLevel::info, "Prism runtime initialization finished.");
    return true;
  } catch (...) {
    prism::detail::log(prism::detail::LogLevel::error, "Prism runtime initialization failed.");
    g_manager.reset();
    return false;
  }
}

extern "C" __declspec(dllexport) void __cdecl PrismShutdown() noexcept {
  g_manager.reset();
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
  // No complex work under the loader lock.
  if (reason == DLL_PROCESS_DETACH) {
    // Explicit PrismShutdown is preferred; static destruction is not performed here.
  }
  return TRUE;
}
