#include <Windows.h>
#include <filesystem>
#include <string>

namespace {
DWORD WINAPI load_prism_runtime(void*) {
  wchar_t self[MAX_PATH]{};
  const DWORD length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), self, MAX_PATH);
  if (!length || length >= MAX_PATH) return 1;
  const auto runtime = std::filesystem::path(self).parent_path() / L"PrismRuntime.dll";
  if (!std::filesystem::exists(runtime)) return 2;
  LoadLibraryW(runtime.c_str());
  return 0;
}
}

extern "C" IMAGE_DOS_HEADER __ImageBase;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(instance);
    HANDLE thread = CreateThread(nullptr, 0, load_prism_runtime, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
  }
  return TRUE;
}
