# Prism

Prism is a Windows-native mod-loader project for Geometry Dash 2.209. It is being built around a C++ runtime, a native mod API, a desktop launcher, defensive diagnostics, reproducible Windows builds, and a straightforward installer.

> **Status:** The first implementation branch contains a native launcher/injector, a runtime plugin loader, an example mod, API tests, and a Windows CI/installer pipeline. It is not yet a finished or game-tested release. In-game UI, reliable integration with Geometry Dash 2.209, and automatic updating remain outstanding.

## Current implementation

- Windows x64 C++20 project built with CMake and MSVC.
- Launcher with game executable selection, launch/runtime-load action, mod enable/disable controls, log-folder shortcut, and safe-mode reset.
- Native DLL API v1 with load/unload callbacks and host logging.
- Plugin scan with descriptor/API validation and duplicate-ID rejection.
- Diagnostic log, best-effort minidump capture, and safe mode that skips mods on the next launch after an unhandled exception.
- GitHub Actions build/test/package workflow and an Inno Setup installer definition.

## Build

Use Visual Studio 2022 with the **Desktop development with C++** workload and CMake 3.24 or newer:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

See [BUILDING.md](docs/BUILDING.md), [architecture](docs/ARCHITECTURE.md), and [mod API](docs/MOD_API.md).

## Important limitations

The launcher can attempt to load the runtime DLL into a running Geometry Dash process, but successful DLL loading does not prove game compatibility. The current runtime does not yet hook game rendering or expose an in-game menu. Automatic updates, a polished mod catalogue, and end-to-end testing against a real Geometry Dash 2.209 installation still need implementation and verification. Do not label builds stable until those checks pass.

Native mods are arbitrary code running with the game's privileges. Install only mods you trust. Prism is not affiliated with or endorsed by RobTop Games.
