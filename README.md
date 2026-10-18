# Prism

Prism is a Windows-native mod loader and mod manager for Geometry Dash 2.209. The project is being built around a small C++ runtime, a documented native mod API, safe mod lifecycle management, reproducible Windows builds, and a straightforward installer.

> **Build status:** Initial implementation is in progress. A successful CI build is not the same as verified compatibility with a local Geometry Dash installation. Releases will be labelled stable only after runtime and installer checks have passed on Windows.

## Goals

- Native C++ runtime and documented plugin ABI.
- Discoverable, individually enableable/disableable mods.
- Defensive loading, diagnostics, and crash reports.
- Windows CI builds and versioned distributable artifacts.
- Simple install/uninstall experience.

## Build

Use Visual Studio 2022 with the **Desktop development with C++** workload and CMake 3.24 or newer. Configure and build with:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Project status

See [BUILDING.md](docs/BUILDING.md), [architecture](docs/ARCHITECTURE.md), and [mod API](docs/MOD_API.md). Do not install unsigned or untrusted third-party mods. Prism is not affiliated with or endorsed by RobTop Games.
