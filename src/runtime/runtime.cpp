#include <Windows.h>
#include <filesystem>
#include <memory>
#include "logger.h"
#include "plugin_manager.h"

extern "C" __declspec(dllexport) bool __cdecl PrismInitialize() noexcept;

namespace {
prism::detail::PluginManager* g_manager = nullptr;

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
DWORD WINAPI initialize_worker(void*) {
  PrismInitialize();
  return 0;
}
}

extern "C" __declspec(dllexport) bool __cdecl PrismInitialize() noexcept {
  try {
    if (g_manager) return true;
    prism::detail::log(prism::detail::LogLevel::info, "Prism runtime initialization started.");
    auto* manager = new prism::detail::PluginManager();
    manager->load_directory(install_root() / L"mods");
    g_manager = manager;
    prism::detail::log(prism::detail::LogLevel::info, "Prism runtime initialization finished.");
    return true;
  } catch (...) {
    prism::detail::log(prism::detail::LogLevel::error, "Prism runtime initialization failed.");
    return false;
  }
}

extern "C" __declspec(dllexport) void __cdecl PrismShutdown() noexcept {
  auto* manager = g_manager;
  g_manager = nullptr;
  delete manager;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(instance);
    HANDLE worker = CreateThread(nullptr, 0, initialize_worker, nullptr, 0, nullptr);
    if (worker) CloseHandle(worker);
  }
  // Do not unload mods or wait on threads while holding the Windows loader lock.
  // Process teardown reclaims resources; PrismShutdown is available for orderly shutdown.
  return TRUE;
}
