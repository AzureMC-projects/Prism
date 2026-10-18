# Prism

![Prism — Geometry Dash mod loader](assets/prism-banner.svg)

Prism is a Windows-native mod-loader project for Geometry Dash 2.209, built around a C++ runtime, native mod API, diagnostic recovery, and Windows packaging.

> **Status:** Early implementation. The native loader foundation, sample mod, launcher, brand assets, and installer pipeline are in progress. The in-game menu/render integration, reliable compatibility with Geometry Dash 2.209, automatic updates, and full end-to-end testing are not complete. Do not treat this as a stable release.

## Brand assets

- App/repository icon: [assets/prism-icon.svg](assets/prism-icon.svg)
- Repository banner: [assets/prism-banner.svg](assets/prism-banner.svg)
- The Windows launcher icon is generated from a checked-in script at build/configure time.

## Current implementation

- Windows x64 C++20 project built with CMake and MSVC.
- Game-folder bootstrap intended to load Prism automatically when Geometry Dash starts, plus a lightweight in-game overlay with an F10 toggle.
- Optional native launcher remains available for diagnostics; it is not the normal startup path.
- Native DLL API v1 with load/unload callbacks and host logging.
- Plugin scan with descriptor/API validation and duplicate-ID rejection.
- Diagnostic log, best-effort minidump capture, and safe mode after an unhandled exception.
- GitHub Actions build/test/package workflow and an Inno Setup installer.

## Install into Geometry Dash

The installer targets the **folder containing GeometryDash.exe** and installs a `version.dll` startup proxy, Prism runtime, and sample mod. After installation, start Geometry Dash normally through Steam; no separate launcher is intended to be required. Prism attempts to show a small `P` button over the game window. Press **F10** to toggle the menu. The overlay provides a mod list, enable/disable-by-file-rename controls, and shortcuts to the mods/log folders. Mod state changes apply after restarting the game.

**Compatibility note:** this startup mechanism uses a local `version.dll` proxy. If another `version.dll` already exists in the game folder, the installer refuses to overwrite it because two proxy loaders can conflict. The overlay is a native topmost window aligned over the game, not a hook into Geometry Dash's internal Cocos rendering engine. Test it against your game build and report issues before relying on it.

## Uninstall

Use **Windows Settings → Apps → Installed apps → Prism → Uninstall**, or choose **Uninstall Prism** from the Start menu. The uninstaller removes Prism's installed files and shortcuts. It does not intentionally delete Geometry Dash, saves, or unrelated files/mods in the mods folder; that folder is removed only if empty. Back up custom files before uninstalling as a general precaution.

## Build

Use Visual Studio 2022 with the **Desktop development with C++** workload, CMake 3.24 or newer, and Python 3:

\`\`\`powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
\`\`\`

See [BUILDING.md](docs/BUILDING.md), [architecture](docs/ARCHITECTURE.md), and [mod API](docs/MOD_API.md).

## Important limitations

The current runtime does not yet hook game rendering or expose an in-game mod menu/button. Automatic updates, polished mod discovery, and end-to-end testing against a real Geometry Dash 2.209 installation remain outstanding. The loader's injection path is best-effort and must be verified on the target machine.

Native mods are arbitrary code running with the game's privileges. Install only mods you trust. Prism is not affiliated with or endorsed by RobTop Games.
