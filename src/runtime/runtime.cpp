#include <Windows.h>
#include <DbgHelp.h>
#include <filesystem>
#include <fstream>
#include "logger.h"
#include "plugin_manager.h"

extern "C" __declspec(dllexport) bool __cdecl PrismInitialize() noexcept;

namespace {
prism::detail::PluginManager* g_manager = nullptr;
LPTOP_LEVEL_EXCEPTION_FILTER g_previous_filter = nullptr;

std::filesystem::path data_directory() {
  wchar_t local[MAX_PATH]{};
  const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return std::filesystem::current_path() / L"Prism";
  return std::filesystem::path(local) / L"Prism";
}
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

LONG WINAPI prism_exception_filter(EXCEPTION_POINTERS* info) {
  try {
    const auto root = data_directory();
    std::filesystem::create_directories(root / L"logs");
    std::ofstream marker(root / L"safe-mode.flag", std::ios::trunc);
    marker << "A previous game process crashed while Prism was loaded. Mods are disabled on next startup until safe mode is cleared.\n";
    marker.close();

    wchar_t dump_name[80]{};
    swprintf_s(dump_name, L"crash-%lu.dmp", GetCurrentProcessId());
    const auto dump_path = root / L"logs" / dump_name;
    HANDLE dump = CreateFileW(dump_path.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dump != INVALID_HANDLE_VALUE) {
      MINIDUMP_EXCEPTION_INFORMATION exception{};
      exception.ThreadId = GetCurrentThreadId();
      exception.ExceptionPointers = info;
      exception.ClientPointers = FALSE;
      MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump,
          MiniDumpNormal, info ? &exception : nullptr, nullptr, nullptr);
      CloseHandle(dump);
    }
    prism::detail::log(prism::detail::LogLevel::error,
        "Unhandled exception observed; crash dump attempted and safe mode enabled for the next launch.");
  } catch (...) {}
  if (g_previous_filter) return g_previous_filter(info);
  return EXCEPTION_EXECUTE_HANDLER;
}

DWORD WINAPI initialize_worker(void*) {
  PrismInitialize();
  return 0;
}
}

extern "C" __declspec(dllexport) bool __cdecl PrismInitialize() noexcept {
  try {
    if (g_manager) return true;
    g_previous_filter = SetUnhandledExceptionFilter(prism_exception_filter);
    prism::detail::log(prism::detail::LogLevel::info, "Prism runtime initialization started.");
    auto* manager = new prism::detail::PluginManager();
    if (std::filesystem::exists(data_directory() / L"safe-mode.flag")) {
      prism::detail::log(prism::detail::LogLevel::warning,
          "Safe mode active: mods were not loaded after a previous crash. Clear safe-mode.flag to re-enable mods.");
    } else {
      manager->load_directory(install_root() / L"mods");
    }
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
  if (g_previous_filter) {
    SetUnhandledExceptionFilter(g_previous_filter);
    g_previous_filter = nullptr;
  }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(instance);
    HANDLE worker = CreateThread(nullptr, 0, initialize_worker, nullptr, 0, nullptr);
    if (worker) CloseHandle(worker);
  }
  // Do not unload mods or wait on threads while holding the Windows loader lock.
  return TRUE;
}
