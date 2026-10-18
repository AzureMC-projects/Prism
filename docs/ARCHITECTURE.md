# Architecture

## Components

- **PrismRuntime**: native Windows DLL with explicit `PrismInitialize` and `PrismShutdown` exports.
- **Mod ABI v1**: C-compatible export convention and versioned descriptor in `include/prism/mod_api.h`.
- **Plugin manager**: scans the runtime's adjacent `mods` directory, validates descriptors, rejects duplicate IDs/API mismatches, and unloads in reverse load order.
- **Diagnostics**: appends logs to `%LOCALAPPDATA%\\Prism\\logs\\Prism.log` (falls back to the current directory if LOCALAPPDATA is unavailable).
- **Sample mod**: a buildable DLL demonstrating the API.
- **Windows CI**: configures, builds, tests, and packages x64 Release outputs.

## Important current boundary

This initial implementation is a runtime/plugin foundation, not yet a complete Geometry Dash integration. It does not currently inject itself into Geometry Dash, hook game rendering, provide an in-game menu, automatically download mods, or verify compatibility against the commercial game executable. Those features require separate integration and real Windows/game testing. Do not describe this commit as a finished or game-tested release.

## Loading contract

A mod DLL exports `PrismGetModDescriptor` with C linkage and `__cdecl`. The descriptor and all strings/function pointers must remain valid for the lifetime of the loaded DLL. `on_load` returns false to reject initialization; `on_unload` should release mod-owned resources. Native plugins execute with the privileges of the host process and must be treated as trusted code.

## Safety

- Do not load mods from untrusted sources.
- ABI checks reduce accidental incompatibility; they are not a security sandbox.
- Avoid complex work in `DllMain`; call explicit exports from a future host/bootstrapper.
