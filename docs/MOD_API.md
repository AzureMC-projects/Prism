# Prism Mod API v1

Prism mods are native Windows DLLs compiled for the same architecture as Prism (currently x64). Include `prism/mod_api.h` and export the descriptor function:

```cpp
#include "prism/mod_api.h"

namespace {
bool PRISM_CALL on_load(const prism::HostApi* host) {
  if (!host || host->api_version != prism::kModApiVersion) return false;
  host->log_info("My mod loaded");
  return true;
}
void PRISM_CALL on_unload() {}
const prism::ModDescriptor descriptor{
  sizeof(prism::ModDescriptor), prism::kModApiVersion,
  "com.example.my-mod", "My Mod", "1.0.0", on_load, on_unload
};
}
extern "C" __declspec(dllexport)
const prism::ModDescriptor* __cdecl PrismGetModDescriptor() {
  return &descriptor;
}
```

Build the DLL with MSVC x64. Place it in the runtime-adjacent `mods` folder. The loader checks the descriptor size, API version, required fields, duplicate IDs, and lifecycle callbacks.

## Rules

- Use a globally unique, stable reverse-domain ID.
- Keep the descriptor, strings, and callback code alive until unload.
- Return false if initialization cannot complete.
- Release resources in `on_unload`.
- Never throw exceptions across the ABI boundary.
- Do not assume a sandbox: a native DLL can execute arbitrary code with host privileges.
- This API currently provides lifecycle and logging only. Game hooks, UI, settings, permissions, and dependency management are not yet implemented.
