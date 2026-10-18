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

Build output includes `PrismLauncher.exe`, `PrismRuntime.dll`, `PrismHelloMod.dll`, and the API test executable. The launcher provides a Geometry Dash executable picker, launch/injection attempt, per-mod enable/disable controls, and log/safe-mode shortcuts.

## Diagnostics and recovery

- Runtime log: `%LOCALAPPDATA%\\Prism\\logs\\Prism.log`
- Best-effort crash dump: `%LOCALAPPDATA%\\Prism\\logs\\crash-<pid>.dmp`
- Safe-mode marker: `%LOCALAPPDATA%\\Prism\\safe-mode.flag`

After an unhandled exception, Prism attempts to write a minidump and enables safe mode for the next runtime initialization so mods are skipped. Open Prism and choose **Clear Safe Mode** only after addressing the likely cause. This recovery path has not yet been verified against Geometry Dash and may not capture every crash.

## Installer

The Windows workflow builds an Inno Setup installer. The installer places Prism under Program Files and creates a Start Menu shortcut. It does not modify Geometry Dash installation files. An installer build is not proof that injection or game compatibility has passed validation.

## Troubleshooting

- Use x64 throughout; mixing x86 and x64 DLLs will fail.
- Start Geometry Dash normally through Steam if Prism cannot start it directly.
- If injection is denied, run Prism with the same permission level as the game.
- Never test new native mods against a valuable save without backups.
