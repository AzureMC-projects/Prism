#include <Windows.h>
#include <TlHelp32.h>
#include <commdlg.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <vector>

namespace {
constexpr int kPath = 1001, kBrowse = 1002, kLaunch = 1003;
constexpr int kMods = 1004, kToggle = 1005, kRefresh = 1006, kOpenLogs = 1007, kClearSafe = 1008;
HWND g_path{}, g_mod_list{};
std::vector<std::filesystem::path> g_mod_paths;

std::wstring module_directory() {
  wchar_t buffer[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
  if (!n || n >= MAX_PATH) return std::filesystem::current_path().wstring();
  return std::filesystem::path(buffer).parent_path().wstring();
}
std::filesystem::path mods_directory() {
  return std::filesystem::path(module_directory()) / L"mods";
}
std::filesystem::path data_directory() {
  wchar_t local[MAX_PATH]{};
  const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return std::filesystem::current_path() / L"Prism";
  return std::filesystem::path(local) / L"Prism";
}
bool is_disabled_mod(const std::filesystem::path& p) {
  return p.extension() == L".disabled" && p.stem().extension() == L".dll";
}
void refresh_mods() {
  SendMessageW(g_mod_list, LB_RESETCONTENT, 0, 0);
  g_mod_paths.clear();
  try {
    std::filesystem::create_directories(mods_directory());
    for (const auto& e : std::filesystem::directory_iterator(mods_directory())) {
      if (!e.is_regular_file()) continue;
      const auto p = e.path();
      const bool enabled = p.extension() == L".dll";
      if (!enabled && !is_disabled_mod(p)) continue;
      g_mod_paths.push_back(p);
      const std::wstring label = (enabled ? L"[ON]  " : L"[OFF] ") + p.filename().wstring();
      SendMessageW(g_mod_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
  } catch (...) {
    MessageBoxW(nullptr, L"Could not read the mods directory.", L"Prism", MB_OK | MB_ICONWARNING);
  }
}
DWORD find_game_process() {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) return 0;
  PROCESSENTRY32W entry{ sizeof(entry) };
  DWORD pid = 0;
  if (Process32FirstW(snapshot, &entry)) {
    do {
      if (_wcsicmp(entry.szExeFile, L"GeometryDash.exe") == 0) {
        pid = entry.th32ProcessID;
        break;
      }
    } while (Process32NextW(snapshot, &entry));
  }
  CloseHandle(snapshot);
  return pid;
}
bool inject_runtime(DWORD pid, const std::wstring& dll_path, std::wstring& error) {
  HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
      PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
  if (!process) { error = L"Could not open Geometry Dash. Try running Prism with the same permissions as the game."; return false; }
  const SIZE_T bytes = (dll_path.size() + 1) * sizeof(wchar_t);
  void* remote_path = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!remote_path) { error = L"Could not allocate memory in the game process."; CloseHandle(process); return false; }
  SIZE_T written = 0;
  bool ok = WriteProcessMemory(process, remote_path, dll_path.c_str(), bytes, &written) && written == bytes;
  HANDLE thread = nullptr;
  if (ok) {
    auto kernel = GetModuleHandleW(L"kernel32.dll");
    auto load_library = kernel ? GetProcAddress(kernel, "LoadLibraryW") : nullptr;
    if (!load_library) { error = L"Could not locate LoadLibraryW."; ok = false; }
    else {
      thread = CreateRemoteThread(process, nullptr, 0,
          reinterpret_cast<LPTHREAD_START_ROUTINE>(load_library), remote_path, 0, nullptr);
      if (!thread) { error = L"Windows refused to start the runtime loader thread."; ok = false; }
    }
  } else error = L"Could not write the runtime path into the game process.";
  if (thread) {
    const DWORD wait = WaitForSingleObject(thread, 15000);
    DWORD exit_code = 0;
    if (wait != WAIT_OBJECT_0 || !GetExitCodeThread(thread, &exit_code) || exit_code == 0) {
      error = L"PrismRuntime could not be loaded. Check architecture and file permissions.";
      ok = false;
    }
    CloseHandle(thread);
  }
  VirtualFreeEx(process, remote_path, 0, MEM_RELEASE);
  CloseHandle(process);
  return ok;
}
void show_error(const std::wstring& message) {
  MessageBoxW(nullptr, message.c_str(), L"Prism", MB_OK | MB_ICONERROR);
}
void browse_game() {
  wchar_t path[MAX_PATH]{};
  OPENFILENAMEW ofn{ sizeof(ofn) };
  ofn.hwndOwner = GetActiveWindow();
  ofn.lpstrFilter = L"Geometry Dash executable (GeometryDash.exe)\0GeometryDash.exe\0Executables (*.exe)\0*.exe\0All files (*.*)\0*.*\0";
  ofn.lpstrFile = path;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrTitle = L"Select GeometryDash.exe";
  if (GetOpenFileNameW(&ofn)) SetWindowTextW(g_path, path);
}
void toggle_selected_mod() {
  const LRESULT selected = SendMessageW(g_mod_list, LB_GETCURSEL, 0, 0);
  if (selected == LB_ERR || static_cast<size_t>(selected) >= g_mod_paths.size()) {
    MessageBoxW(nullptr, L"Select a mod first.", L"Prism", MB_OK | MB_ICONINFORMATION);
    return;
  }
  const auto source = g_mod_paths[static_cast<size_t>(selected)];
  std::filesystem::path destination;
  if (source.extension() == L".dll") destination = source.wstring() + L".disabled";
  else {
    std::wstring name = source.wstring();
    name.resize(name.size() - std::wstring(L".disabled").size());
    destination = name;
  }
  std::error_code ec;
  std::filesystem::rename(source, destination, ec);
  if (ec) {
    show_error(L"Could not change mod state. Close any program using the file and try again.");
    return;
  }
  refresh_mods();
}
void clear_safe_mode() {
  std::error_code ec;
  const bool removed = std::filesystem::remove(data_directory() / L"safe-mode.flag", ec);
  if (ec) show_error(L"Could not clear safe mode. Check the permissions of your local Prism data folder.");
  else MessageBoxW(nullptr, removed ? L"Safe mode cleared. Mods will be attempted on the next game launch." : L"Safe mode is not currently active.", L"Prism", MB_OK | MB_ICONINFORMATION);
}
void launch_and_inject() {
  wchar_t game[MAX_PATH]{};
  GetWindowTextW(g_path, game, MAX_PATH);
  if (!game[0] || !std::filesystem::exists(game)) {
    show_error(L"Select a valid GeometryDash.exe first.");
    return;
  }
  const auto runtime = std::filesystem::path(module_directory()) / L"PrismRuntime.dll";
  if (!std::filesystem::exists(runtime)) {
    show_error(L"PrismRuntime.dll is missing. Reinstall Prism or restore the file from the build package.");
    return;
  }
  DWORD pid = find_game_process();
  HANDLE launched = nullptr;
  if (!pid) {
    STARTUPINFOW si{ sizeof(si) };
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(game, nullptr, nullptr, nullptr, FALSE, 0, nullptr,
        std::filesystem::path(game).parent_path().c_str(), &si, &pi)) {
      show_error(L"Could not start Geometry Dash. Start it normally through Steam, then click Launch / Load Prism.");
      return;
    }
    pid = pi.dwProcessId;
    launched = pi.hProcess;
    CloseHandle(pi.hThread);
    Sleep(5000);
  }
  std::wstring error;
  const bool ok = inject_runtime(pid, runtime.wstring(), error);
  if (launched) CloseHandle(launched);
  if (!ok) show_error(error);
  else MessageBoxW(nullptr, L"PrismRuntime was loaded into the game process. This confirms DLL loading only; game compatibility and in-game features are not yet verified.", L"Prism", MB_OK | MB_ICONINFORMATION);
}
void open_logs() {
  const auto logs = data_directory() / L"logs";
  std::error_code ec;
  std::filesystem::create_directories(logs, ec);
  ShellExecuteW(nullptr, L"open", logs.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_CREATE) {
    CreateWindowW(L"STATIC", L"Prism", WS_CHILD | WS_VISIBLE, 24, 14, 340, 34, hwnd, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Geometry Dash executable", WS_CHILD | WS_VISIBLE, 24, 52, 340, 22, hwnd, nullptr, nullptr, nullptr);
    g_path = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 24, 76, 350, 26, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPath)), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 386, 76, 100, 26, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBrowse)), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Launch / Load Prism", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 24, 112, 462, 34, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLaunch)), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Mods (rename .dll to .dll.disabled to disable)", WS_CHILD | WS_VISIBLE, 24, 158, 462, 20, hwnd, nullptr, nullptr, nullptr);
    g_mod_list = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 24, 180, 462, 112, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMods)), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Enable / Disable Selected", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 24, 304, 220, 30, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kToggle)), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 254, 304, 100, 30, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRefresh)), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Open Logs", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 364, 304, 122, 30, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kOpenLogs)), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Clear Safe Mode", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 24, 346, 462, 30, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kClearSafe)), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Native runtime foundation. In-game UI and game compatibility are not yet verified.", WS_CHILD | WS_VISIBLE, 24, 388, 480, 32, hwnd, nullptr, nullptr, nullptr);
    refresh_mods();
    return 0;
  }
  if (msg == WM_COMMAND) {
    switch (LOWORD(wp)) {
      case kBrowse: browse_game(); return 0;
      case kLaunch: launch_and_inject(); return 0;
      case kToggle: toggle_selected_mod(); return 0;
      case kRefresh: refresh_mods(); return 0;
      case kOpenLogs: open_logs(); return 0;
      case kClearSafe: clear_safe_mode(); return 0;
    }
  }
  if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
  return DefWindowProcW(hwnd, msg, wp, lp);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
  const wchar_t cls[] = L"PrismLauncherWindow";
  WNDCLASSW wc{};
  wc.lpfnWndProc = window_proc;
  wc.hInstance = instance;
  wc.lpszClassName = cls;
  wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  if (!RegisterClassW(&wc)) return 1;
  HWND hwnd = CreateWindowExW(0, cls, L"Prism - Geometry Dash Mod Loader", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT, CW_USEDEFAULT, 530, 475, nullptr, nullptr, instance, nullptr);
  if (!hwnd) return 1;
  ShowWindow(hwnd, show);
  UpdateWindow(hwnd);
  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return static_cast<int>(msg.wParam);
}
