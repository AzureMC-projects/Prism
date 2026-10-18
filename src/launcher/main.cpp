#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <commdlg.h>
#include <filesystem>
#include <string>
#include <vector>

namespace {
constexpr int kPath = 1001, kBrowse = 1002, kLaunch = 1003;
HWND g_path{};

std::wstring module_directory() {
  wchar_t buffer[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
  if (!n || n >= MAX_PATH) return std::filesystem::current_path().wstring();
  return std::filesystem::path(buffer).parent_path().wstring();
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
      show_error(L"Could not start Geometry Dash. Start it normally through Steam, then try again.\n\nIf the game is already running, Prism will try to find it again.");
      pid = find_game_process();
      if (!pid) return;
    } else {
      pid = pi.dwProcessId;
      launched = pi.hProcess;
      CloseHandle(pi.hThread);
      Sleep(5000);
    }
  }
  std::wstring error;
  const bool ok = inject_runtime(pid, runtime.wstring(), error);
  if (launched) CloseHandle(launched);
  if (!ok) show_error(error);
  else MessageBoxW(nullptr, L"PrismRuntime was loaded into the game process. This confirms DLL loading only; game compatibility and in-game features are not yet verified.", L"Prism", MB_OK | MB_ICONINFORMATION);
}
LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_CREATE) {
    CreateWindowW(L"STATIC", L"Prism", WS_CHILD | WS_VISIBLE, 24, 18, 340, 34, hwnd, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Geometry Dash executable", WS_CHILD | WS_VISIBLE, 24, 64, 340, 22, hwnd, nullptr, nullptr, nullptr);
    g_path = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 24, 88, 350, 28, hwnd, reinterpret_cast<HMENU>(kPath), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Browse…", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 386, 88, 100, 28, hwnd, reinterpret_cast<HMENU>(kBrowse), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Launch / Load Prism", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 24, 136, 462, 38, hwnd, reinterpret_cast<HMENU>(kLaunch), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Native runtime foundation — game features are under development.", WS_CHILD | WS_VISIBLE, 24, 192, 462, 30, hwnd, nullptr, nullptr, nullptr);
    return 0;
  }
  if (msg == WM_COMMAND) {
    switch (LOWORD(wp)) {
      case kBrowse: browse_game(); return 0;
      case kLaunch: launch_and_inject(); return 0;
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
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  if (!RegisterClassW(&wc)) return 1;
  HWND hwnd = CreateWindowExW(0, cls, L"Prism — Geometry Dash Mod Loader", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT, CW_USEDEFAULT, 530, 280, nullptr, nullptr, instance, nullptr);
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
