#include "overlay.h"
#include "logger.h"
#include <Windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <cwchar>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace prism::detail {
namespace {
constexpr COLORREF kTransparent = RGB(0, 0, 0);
constexpr COLORREF kPanel = RGB(22, 25, 39);
constexpr COLORREF kPanelLight = RGB(33, 38, 56);
constexpr COLORREF kAccent = RGB(113, 96, 255);
constexpr COLORREF kText = RGB(242, 244, 255);
constexpr COLORREF kMuted = RGB(157, 166, 190);
constexpr UINT_PTR kTimer = 1;
HWND g_overlay{};
HWND g_game{};
std::filesystem::path g_install_root;
std::filesystem::path g_data_root;
bool g_open{};
bool g_previous_f10{};
int g_selected = -1;
std::vector<std::filesystem::path> g_mods;

std::vector<std::filesystem::path> enumerate_mods() {
  std::vector<std::filesystem::path> result;
  std::error_code ec;
  std::filesystem::create_directories(g_install_root / L"mods", ec);
  for (std::filesystem::directory_iterator it(g_install_root / L"mods", ec), end; !ec && it != end; it.increment(ec)) {
    if (!it->is_regular_file(ec)) continue;
    const auto p = it->path();
    if (p.extension() == L".dll" || (p.extension() == L".disabled" && p.stem().extension() == L".dll"))
      result.push_back(p);
  }
  std::sort(result.begin(), result.end());
  return result;
}
bool enabled(const std::filesystem::path& p) { return p.extension() == L".dll"; }
void text(HDC dc, int x, int y, const wchar_t* value, COLORREF color, int size = 16, int weight = FW_NORMAL) {
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, color);
  HFONT font = CreateFontW(-size, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
  HGDIOBJ old = SelectObject(dc, font);
  TextOutW(dc, x, y, value, static_cast<int>(wcslen(value)));
  SelectObject(dc, old);
  DeleteObject(font);
}
void fill(HDC dc, RECT r, COLORREF color, int radius = 10) {
  HBRUSH brush = CreateSolidBrush(color);
  HPEN pen = CreatePen(PS_SOLID, 1, color);
  HGDIOBJ oldBrush = SelectObject(dc, brush);
  HGDIOBJ oldPen = SelectObject(dc, pen);
  RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
  SelectObject(dc, oldBrush);
  SelectObject(dc, oldPen);
  DeleteObject(brush);
  DeleteObject(pen);
}
void draw_overlay(HDC dc, int width, int height) {
  RECT full{0, 0, width, height};
  HBRUSH clear = CreateSolidBrush(kTransparent);
  FillRect(dc, &full, clear);
  DeleteObject(clear);

  RECT button{std::max(8, width - 58), 12, std::max(50, width - 12), 58};
  fill(dc, button, kAccent, 12);
  text(dc, button.left + 14, button.top + 8, L"P", kText, 25, FW_BOLD);
  if (!g_open) return;

  const int panelW = std::min(560, std::max(360, width - 32));
  const int panelH = std::min(480, std::max(350, height - 32));
  const int left = std::max(12, (width - panelW) / 2);
  const int top = std::max(12, (height - panelH) / 2);
  fill(dc, RECT{left, top, left + panelW, top + panelH}, kPanel, 18);
  fill(dc, RECT{left, top, left + panelW, top + 74}, kPanelLight, 18);
  text(dc, left + 22, top + 15, L"PRISM", kText, 24, FW_BOLD);
  text(dc, left + 22, top + 45, L"Geometry Dash mod menu", kMuted, 13);
  text(dc, left + panelW - 40, top + 18, L"X", kText, 20, FW_BOLD);

  const bool safeMode = std::filesystem::exists(g_data_root / L"safe-mode.flag");
  text(dc, left + 22, top + 88, safeMode ? L"SAFE MODE - mods skipped after a crash" : L"Runtime active  /  F10 toggles this menu",
       safeMode ? RGB(255, 190, 90) : RGB(116, 226, 190), 13, FW_BOLD);
  text(dc, left + 22, top + 116, L"INSTALLED MODS", kMuted, 12, FW_BOLD);

  g_mods = enumerate_mods();
  const int listTop = top + 142;
  const int rowH = 34;
  const int visibleRows = std::max(1, (panelH - 218) / rowH);
  for (int i = 0; i < static_cast<int>(g_mods.size()) && i < visibleRows; ++i) {
    const int y = listTop + i * rowH;
    if (i == g_selected) fill(dc, RECT{left + 14, y, left + panelW - 14, y + 30}, kPanelLight, 7);
    const auto filename = g_mods[static_cast<size_t>(i)].filename().wstring();
    text(dc, left + 26, y + 6, filename.c_str(), kText, 13);
    text(dc, left + panelW - 76, y + 6, enabled(g_mods[static_cast<size_t>(i)]) ? L"ON" : L"OFF",
         enabled(g_mods[static_cast<size_t>(i)]) ? RGB(116, 226, 190) : kMuted, 13, FW_BOLD);
  }
  if (g_mods.empty()) text(dc, left + 26, listTop + 8, L"No mods yet. Add trusted mod DLLs to the mods folder.", kMuted, 12);
  if (static_cast<int>(g_mods.size()) > visibleRows)
    text(dc, left + 22, listTop + visibleRows * rowH + 2, L"More mods exist; manage them from the mods folder.", kMuted, 11);

  const int bottom = top + panelH - 52;
  fill(dc, RECT{left + 18, bottom, left + 158, bottom + 34}, kAccent, 8);
  text(dc, left + 27, bottom + 8, L"Enable / Disable", kText, 12, FW_BOLD);
  fill(dc, RECT{left + 166, bottom, left + 274, bottom + 34}, kPanelLight, 8);
  text(dc, left + 176, bottom + 8, L"Mods folder", kText, 12, FW_BOLD);
  fill(dc, RECT{left + 282, bottom, left + 382, bottom + 34}, kPanelLight, 8);
  text(dc, left + 300, bottom + 8, L"Open logs", kText, 12, FW_BOLD);
  fill(dc, RECT{left + 390, bottom, left + 536, bottom + 34}, kPanelLight, 8);
  text(dc, left + 399, bottom + 8, L"Clear safe mode", kText, 12, FW_BOLD);
  text(dc, left + 22, top + panelH - 12, L"Mod changes apply next game launch. Only use mods you trust.", kMuted, 11);
}
BOOL CALLBACK find_game_window(HWND hwnd, LPARAM data) {
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  if (pid == GetCurrentProcessId() && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr) {
    *reinterpret_cast<HWND*>(data) = hwnd;
    return FALSE;
  }
  return TRUE;
}
void sync_position() {
  if (!g_game || !IsWindow(g_game) || !g_overlay) return;
  RECT r{};
  if (!GetClientRect(g_game, &r)) return;
  if (IsIconic(g_game)) { ShowWindow(g_overlay, SW_HIDE); return; }
  POINT p{r.left, r.top};
  ClientToScreen(g_game, &p);
  SetWindowPos(g_overlay, HWND_TOPMOST, p.x, p.y, r.right - r.left, r.bottom - r.top,
      SWP_NOACTIVATE | SWP_SHOWWINDOW);
}
void toggle_mod() {
  g_mods = enumerate_mods();
  if (g_selected < 0 || g_selected >= static_cast<int>(g_mods.size())) return;
  const auto source = g_mods[static_cast<size_t>(g_selected)];
  const auto destination = enabled(source)
      ? std::filesystem::path(source.wstring() + L".disabled")
      : std::filesystem::path(source.wstring().substr(0, source.wstring().size() - std::wstring(L".disabled").size()));
  std::error_code ec;
  std::filesystem::rename(source, destination, ec);
  if (ec) log(LogLevel::warning, "In-game menu could not change mod state; check file permissions.");
  else log(LogLevel::info, "Mod state changed from the in-game menu; restart the game to apply.");
  g_mods = enumerate_mods();
  g_selected = -1;
  InvalidateRect(g_overlay, nullptr, TRUE);
}
LRESULT CALLBACK overlay_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
  switch (message) {
    case WM_TIMER: {
      const bool f10 = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
      if (f10 && !g_previous_f10) { g_open = !g_open; InvalidateRect(hwnd, nullptr, TRUE); }
      g_previous_f10 = f10;
      if (!IsWindow(g_game)) { DestroyWindow(hwnd); return 0; }
      sync_position();
      return 0;
    }
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    case WM_LBUTTONDOWN: {
      const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
      RECT client{}; GetClientRect(hwnd, &client);
      if (x >= client.right - 64 && y >= 8 && y <= 64) {
        g_open = !g_open;
      } else if (g_open) {
        const int width = client.right, height = client.bottom;
        const int panelW = std::min(560, std::max(360, width - 32));
        const int panelH = std::min(480, std::max(350, height - 32));
        const int left = std::max(12, (width - panelW) / 2);
        const int top = std::max(12, (height - panelH) / 2);
        if (x >= left + panelW - 55 && x <= left + panelW && y >= top && y <= top + 74) g_open = false;
        else {
          const int listTop = top + 142, rowH = 34;
          const int visibleRows = std::max(1, (panelH - 218) / rowH);
          if (x >= left + 14 && x <= left + panelW - 14 && y >= listTop &&
              y < listTop + visibleRows * rowH) {
            const int index = (y - listTop) / rowH;
            g_mods = enumerate_mods();
            g_selected = index < static_cast<int>(g_mods.size()) ? index : -1;
          } else if (y >= top + panelH - 52 && y <= top + panelH - 18) {
            if (x >= left + 18 && x <= left + 158) toggle_mod();
            else if (x >= left + 166 && x <= left + 274)
              ShellExecuteW(nullptr, L"open", (g_install_root / L"mods").c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            else if (x >= left + 282 && x <= left + 382) {
              std::error_code ec; std::filesystem::create_directories(g_data_root / L"logs", ec);
              ShellExecuteW(nullptr, L"open", (g_data_root / L"logs").c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            } else if (x >= left + 390 && x <= left + 536) {
              std::error_code ec;
              std::filesystem::remove(g_data_root / L"safe-mode.flag", ec);
              if (ec) log(LogLevel::warning, "Could not clear Prism safe-mode marker.");
              else log(LogLevel::info, "Safe mode marker cleared from the in-game menu.");
            }
          }
        }
      }
      InvalidateRect(hwnd, nullptr, TRUE);
      return 0;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      HDC dc = BeginPaint(hwnd, &ps);
      RECT r{}; GetClientRect(hwnd, &r);
      draw_overlay(dc, r.right, r.bottom);
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_DESTROY: KillTimer(hwnd, kTimer); g_overlay = nullptr; PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(hwnd, message, wp, lp);
}
DWORD WINAPI overlay_thread(void*) {
  EnumWindows(find_game_window, reinterpret_cast<LPARAM>(&g_game));
  if (!g_game) {
    for (int i = 0; i < 60 && !g_game; ++i) { Sleep(1000); EnumWindows(find_game_window, reinterpret_cast<LPARAM>(&g_game)); }
  }
  if (!g_game) { log(LogLevel::warning, "Prism overlay could not find the Geometry Dash game window."); return 1; }
  const wchar_t cls[] = L"PrismInGameOverlay";
  WNDCLASSW wc{};
  wc.lpfnWndProc = overlay_proc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = cls;
  wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  RegisterClassW(&wc);
  RECT r{}; GetClientRect(g_game, &r);
  POINT p{0, 0}; ClientToScreen(g_game, &p);
  g_overlay = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
      cls, L"Prism In-Game Menu", WS_POPUP, p.x, p.y, r.right, r.bottom, nullptr, nullptr,
      wc.hInstance, nullptr);
  if (!g_overlay) { log(LogLevel::error, "Could not create Prism overlay window."); return 1; }
  SetLayeredWindowAttributes(g_overlay, kTransparent, 0, LWA_COLORKEY);
  SetTimer(g_overlay, kTimer, 33, nullptr);
  sync_position();
  ShowWindow(g_overlay, SW_SHOWNOACTIVATE);
  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
  return 0;
}
}
void start_overlay(const std::filesystem::path& install_root, const std::filesystem::path& data_root) noexcept {
  try {
    g_install_root = install_root;
    g_data_root = data_root;
    HANDLE thread = CreateThread(nullptr, 0, overlay_thread, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
    else log(LogLevel::error, "Could not start Prism overlay thread.");
  } catch (...) { log(LogLevel::error, "Prism overlay startup failed."); }
}
}
