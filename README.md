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
- Native launcher with game executable selection, runtime-load action, mod enable/disable controls, log-folder shortcut, and safe-mode reset.
- Native DLL API v1 with load/unload callbacks and host logging.
- Plugin scan with descriptor/API validation and duplicate-ID rejection.
- Diagnostic log, best-effort minidump capture, and safe mode after an unhandled exception.
- GitHub Actions build/test/package workflow and an Inno Setup installer.

## Install into Geometry Dash

The installer targets the **folder containing GeometryDash.exe** and places Prism's launcher/runtime and sample mod there, similar to the game-folder layout used by mod loaders. If Geometry Dash is installed in a custom Steam library, select that folder in the installer.

The installer copies files into the game folder; setup itself does **not** inject a DLL into a running game. Use Prism Launcher to attempt runtime loading after installation. Successful DLL loading alone does not prove game compatibility or create an in-game menu yet.

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
