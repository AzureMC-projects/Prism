#include <Windows.h>
#include <winver.h>
#include <filesystem>
#include <string>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {
HMODULE g_system_version{};
FARPROC original(const char* name) {
  if (!g_system_version) {
    wchar_t directory[MAX_PATH]{};
    const UINT length = GetSystemDirectoryW(directory, MAX_PATH);
    if (!length || length >= MAX_PATH) return nullptr;
    std::filesystem::path path(directory);
    path /= L"version.dll";
    g_system_version = LoadLibraryW(path.c_str());
  }
  return g_system_version ? GetProcAddress(g_system_version, name) : nullptr;
}
DWORD WINAPI load_prism_runtime(void*) {
  wchar_t self[MAX_PATH]{};
  const DWORD length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), self, MAX_PATH);
  if (!length || length >= MAX_PATH) return 1;
  const auto runtime = std::filesystem::path(self).parent_path() / L"PrismRuntime.dll";
  if (!std::filesystem::exists(runtime)) return 2;
  if (!LoadLibraryW(runtime.c_str())) return 3;
  return 0;
}
}
#define PRISM_FORWARD(ret, name, params, args) extern "C" ret WINAPI Prism##name params { using Fn = ret (WINAPI*) params; auto fn = reinterpret_cast<Fn>(original(#name)); if (!fn) return static_cast<ret>(0); return fn args; }
PRISM_FORWARD(BOOL, GetFileVersionInfoA, (LPCSTR file, DWORD handle, DWORD size, LPVOID data), (file, handle, size, data))
PRISM_FORWARD(BOOL, GetFileVersionInfoW, (LPCWSTR file, DWORD handle, DWORD size, LPVOID data), (file, handle, size, data))
PRISM_FORWARD(DWORD, GetFileVersionInfoSizeA, (LPCSTR file, LPDWORD handle), (file, handle))
PRISM_FORWARD(DWORD, GetFileVersionInfoSizeW, (LPCWSTR file, LPDWORD handle), (file, handle))
PRISM_FORWARD(DWORD, VerFindFileA, (DWORD flags, LPSTR file, LPSTR winDir, LPSTR appDir, LPSTR curDir, PUINT curLen, LPSTR destDir, PUINT destLen), (flags, file, winDir, appDir, curDir, curLen, destDir, destLen))
PRISM_FORWARD(DWORD, VerFindFileW, (DWORD flags, LPWSTR file, LPWSTR winDir, LPWSTR appDir, LPWSTR curDir, PUINT curLen, LPWSTR destDir, PUINT destLen), (flags, file, winDir, appDir, curDir, curLen, destDir, destLen))
PRISM_FORWARD(DWORD, VerInstallFileA, (DWORD flags, LPSTR srcFile, LPSTR dstFile, LPSTR srcDir, LPSTR dstDir, LPSTR curDir, LPSTR tmpFile, PUINT tmpLen), (flags, srcFile, dstFile, srcDir, dstDir, curDir, tmpFile, tmpLen))
PRISM_FORWARD(DWORD, VerInstallFileW, (DWORD flags, LPWSTR srcFile, LPWSTR dstFile, LPWSTR srcDir, LPWSTR dstDir, LPWSTR curDir, LPWSTR tmpFile, PUINT tmpLen), (flags, srcFile, dstFile, srcDir, dstDir, curDir, tmpFile, tmpLen))
PRISM_FORWARD(DWORD, VerLanguageNameA, (DWORD language, LPSTR buffer, DWORD size), (language, buffer, size))
PRISM_FORWARD(DWORD, VerLanguageNameW, (DWORD language, LPWSTR buffer, DWORD size), (language, buffer, size))
PRISM_FORWARD(BOOL, VerQueryValueA, (LPCVOID block, LPCSTR subBlock, LPVOID* buffer, PUINT size), (block, subBlock, buffer, size))
PRISM_FORWARD(BOOL, VerQueryValueW, (LPCVOID block, LPCWSTR subBlock, LPVOID* buffer, PUINT size), (block, subBlock, buffer, size))
PRISM_FORWARD(DWORD, GetFileVersionInfoSizeExA, (DWORD flags, LPCSTR file, LPDWORD handle), (flags, file, handle))
PRISM_FORWARD(DWORD, GetFileVersionInfoSizeExW, (DWORD flags, LPCWSTR file, LPDWORD handle), (flags, file, handle))
PRISM_FORWARD(BOOL, GetFileVersionInfoExA, (DWORD flags, LPCSTR file, DWORD handle, DWORD size, LPVOID data), (flags, file, handle, size, data))
PRISM_FORWARD(BOOL, GetFileVersionInfoExW, (DWORD flags, LPCWSTR file, DWORD handle, DWORD size, LPVOID data), (flags, file, handle, size, data))
#undef PRISM_FORWARD

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(instance);
    HANDLE thread = CreateThread(nullptr, 0, load_prism_runtime, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
  }
  return TRUE;
}
