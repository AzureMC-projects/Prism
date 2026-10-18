#include <Windows.h>
#include <objbase.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <urlmon.h>
#include <filesystem>
#include <string>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")

namespace {
struct ComScope {
  HRESULT result{CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)};
  ~ComScope() { if (SUCCEEDED(result)) CoUninitialize(); }
};
bool game_is_running() {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) return false;
  PROCESSENTRY32W entry{ sizeof(entry) };
  bool found = false;
  if (Process32FirstW(snapshot, &entry)) {
    do {
      if (_wcsicmp(entry.szExeFile, L"GeometryDash.exe") == 0) { found = true; break; }
    } while (Process32NextW(snapshot, &entry));
  }
  CloseHandle(snapshot);
  return found;
}
std::filesystem::path local_data() {
  wchar_t path[MAX_PATH]{};
  const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", path, MAX_PATH);
  if (!n || n >= MAX_PATH) return std::filesystem::temp_directory_path() / L"Prism";
  return std::filesystem::path(path) / L"Prism";
}
}
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  ComScope com;
  if (FAILED(com.result) && com.result != RPC_E_CHANGED_MODE) {
    MessageBoxW(nullptr, L"Windows could not initialize the update service.", L"Prism Updater", MB_OK | MB_ICONERROR);
    return 5;
  }
  wchar_t module[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(nullptr, module, MAX_PATH);
  if (!n || n >= MAX_PATH) {
    MessageBoxW(nullptr, L"Prism could not locate its install folder.", L"Prism Updater", MB_OK | MB_ICONERROR);
    return 1;
  }
  const auto installRoot = std::filesystem::path(module).parent_path();
  const auto updateDir = local_data() / L"updates";
  std::error_code ec;
  std::filesystem::create_directories(updateDir, ec);
  if (ec) {
    MessageBoxW(nullptr, L"Prism could not create its update cache folder.", L"Prism Updater", MB_OK | MB_ICONERROR);
    return 2;
  }
  const auto installer = updateDir / L"Prism-Setup.exe";
  constexpr wchar_t url[] = L"https://github.com/AzureMC-projects/Prism/releases/latest/download/Prism-Setup.exe";
  if (MessageBoxW(nullptr, L"Download the latest Prism installer from the official GitHub releases? The installer will run after Geometry Dash is closed.", L"Prism Updater", MB_YESNO | MB_ICONQUESTION) != IDYES)
    return 0;
  const HRESULT download = URLDownloadToFileW(nullptr, url, installer.c_str(), 0, nullptr);
  if (FAILED(download)) {
    MessageBoxW(nullptr, L"Could not download an official Prism release. A published GitHub release may not exist yet, or your connection may be blocking the download.", L"Prism Updater", MB_OK | MB_ICONWARNING);
    return 3;
  }
  if (MessageBoxW(nullptr, L"Update downloaded. Close Geometry Dash, then click OK to install. Prism will wait until the game exits. Click Cancel to leave the installer in your Prism updates folder.", L"Prism Updater", MB_OKCANCEL | MB_ICONINFORMATION) != IDOK) {
    ShellExecuteW(nullptr, L"open", updateDir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return 0;
  }
  while (game_is_running()) Sleep(1000);
  const std::wstring args = L"/SILENT /NORESTART /DIR=\"" + installRoot.wstring() + L"\"";
  const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"runas", installer.c_str(), args.c_str(), installRoot.c_str(), SW_SHOWNORMAL));
  if (result <= 32) {
    MessageBoxW(nullptr, L"Windows could not start the Prism installer. The downloaded installer is in your Local AppData Prism\\updates folder.", L"Prism Updater", MB_OK | MB_ICONERROR);
    ShellExecuteW(nullptr, L"open", updateDir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return 4;
  }
  return 0;
}
