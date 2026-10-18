# Architecture

## Components

- **PrismLauncher**: native Win32 launcher with game executable selection, process discovery, runtime injection, per-mod enable/disable by changing the file extension, a log-folder shortcut, and safe-mode reset.
- **PrismRuntime**: native Windows DLL with explicit `PrismInitialize` and `PrismShutdown` exports. DLL attach schedules initialization on a worker thread rather than doing plugin work under the loader lock.
- **Mod ABI v1**: versioned descriptor and lifecycle callbacks in `include/prism/mod_api.h`.
- **Plugin manager**: scans the runtime's adjacent `mods` directory, validates descriptors, rejects duplicate IDs/API mismatches, and unloads in reverse load order.
- **Diagnostics/recovery**: logs to `%LOCALAPPDATA%\\Prism\\logs\\Prism.log`. The runtime attempts a minidump and writes `%LOCALAPPDATA%\\Prism\\safe-mode.flag` after an unhandled exception. The next initialization skips loading mods while the flag exists. The launcher can clear this flag.
- **Sample mod**: a buildable DLL demonstrating the API.
- **Windows CI**: configures, builds, tests, and packages x64 Release outputs; tag pushes are configured to publish release assets.

## Important current boundary

This implementation is an early functional foundation, not yet a complete Geometry Dash integration. Injection is best-effort and must be verified against the target game build. It does not currently hook game rendering, provide an in-game menu, automatically download/update mods, or establish compatibility with the commercial game executable. Do not describe it as a finished or game-tested release.

## Loading contract

A mod DLL exports `PrismGetModDescriptor` with C linkage and `__cdecl`. The descriptor and all strings/function pointers must remain valid for the lifetime of the loaded DLL. `on_load` returns false to reject initialization; `on_unload` should release mod-owned resources. Native plugins execute with the privileges of the host process and must be treated as trusted code.

## Safety

- Do not load mods from untrusted sources.
- ABI checks reduce accidental incompatibility; they are not a security sandbox.
- Crash dump capture is best-effort and may be unavailable if the process is severely compromised.
- Avoid complex work in `DllMain`; initialization is delegated to a worker thread.
