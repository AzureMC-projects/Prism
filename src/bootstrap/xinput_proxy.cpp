#include <Windows.h>
#include <Xinput.h>
#include <filesystem>

extern "C" IMAGE_DOS_HEADER __ImageBase;
namespace {
HMODULE g_system_xinput{};
FARPROC original(const char* name) {
  if (!g_system_xinput) {
    wchar_t self[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), self, MAX_PATH);
    if (!n || n >= MAX_PATH) return nullptr;
    const auto systemPath = std::filesystem::path(self).filename();
    wchar_t systemDirectory[MAX_PATH]{};
    const UINT d = GetSystemDirectoryW(systemDirectory, MAX_PATH);
    if (!d || d >= MAX_PATH) return nullptr;
    const auto fullPath = std::filesystem::path(systemDirectory) / systemPath;
    g_system_xinput = LoadLibraryW(fullPath.c_str());
  }
  return g_system_xinput ? GetProcAddress(g_system_xinput, name) : nullptr;
}
DWORD WINAPI load_prism_runtime(void*) {
  wchar_t self[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), self, MAX_PATH);
  if (!n || n >= MAX_PATH) return 1;
  const auto runtime = std::filesystem::path(self).parent_path() / L"PrismRuntime.dll";
  if (!std::filesystem::exists(runtime)) return 2;
  return LoadLibraryW(runtime.c_str()) ? 0 : 3;
}
}
#define PRISM_XINPUT_FORWARD(ret, name, params, args, fallback) extern "C" ret WINAPI Prism##name params { using Fn = ret (WINAPI*) params; auto fn = reinterpret_cast<Fn>(original(#name)); if (!fn) return fallback; return fn args; }
PRISM_XINPUT_FORWARD(DWORD, XInputGetState, (DWORD user, XINPUT_STATE* state), (user, state), ERROR_DEVICE_NOT_CONNECTED)
PRISM_XINPUT_FORWARD(DWORD, XInputSetState, (DWORD user, XINPUT_VIBRATION* vibration), (user, vibration), ERROR_DEVICE_NOT_CONNECTED)
PRISM_XINPUT_FORWARD(DWORD, XInputGetCapabilities, (DWORD user, DWORD flags, XINPUT_CAPABILITIES* capabilities), (user, flags, capabilities), ERROR_DEVICE_NOT_CONNECTED)
PRISM_XINPUT_FORWARD(DWORD, XInputGetDSoundAudioDeviceGuids, (DWORD user, GUID* render, GUID* capture), (user, render, capture), ERROR_DEVICE_NOT_CONNECTED)
PRISM_XINPUT_FORWARD(DWORD, XInputGetBatteryInformation, (DWORD user, BYTE devType, XINPUT_BATTERY_INFORMATION* battery), (user, devType, battery), ERROR_DEVICE_NOT_CONNECTED)
PRISM_XINPUT_FORWARD(DWORD, XInputGetKeystroke, (DWORD user, DWORD reserved, PXINPUT_KEYSTROKE keystroke), (user, reserved, keystroke), ERROR_EMPTY)
PRISM_XINPUT_FORWARD(DWORD, XInputGetAudioDeviceIds, (DWORD user, LPWSTR render, UINT* renderCount, LPWSTR capture, UINT* captureCount), (user, render, renderCount, capture, captureCount), ERROR_DEVICE_NOT_CONNECTED)
#undef PRISM_XINPUT_FORWARD
extern "C" void WINAPI PrismXInputEnable(BOOL enabled) {
  using Fn = void (WINAPI*)(BOOL);
  auto fn = reinterpret_cast<Fn>(original("XInputEnable"));
  if (fn) fn(enabled);
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(instance);
    HANDLE thread = CreateThread(nullptr, 0, load_prism_runtime, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
  }
  return TRUE;
}
