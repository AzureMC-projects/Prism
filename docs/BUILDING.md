# Building Prism

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++
- CMake 3.24+
- Git

## Local build

Run from a Developer PowerShell or a shell where MSVC is available:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Build output includes `PrismRuntime.dll`, `PrismHelloMod.dll`, and the API test executable. CI uploads a ZIP containing the runtime, sample mod, and SDK header.

## Installer

The Windows workflow also builds an Inno Setup installer when its compiler is available. Installer output is a distribution package, not proof that the runtime is injected into Geometry Dash or game-tested.

## Troubleshooting

- Use x64 throughout; mixing x86 and x64 DLLs will fail.
- Inspect `%LOCALAPPDATA%\\Prism\\logs\\Prism.log` for runtime diagnostics.
- Never test new native mods against a valuable save without backups.
