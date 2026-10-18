#include "prism/mod_api.h"

namespace {
const prism::HostApi* g_host{};
bool PRISM_CALL load(const prism::HostApi* host) {
  if (!host || host->api_version != prism::kModApiVersion || !host->log_info) return false;
  g_host = host;
  g_host->log_info("Hello from the Prism example mod.");
  return true;
}
void PRISM_CALL unload() {
  if (g_host && g_host->log_info) g_host->log_info("Prism example mod unloaded.");
  g_host = nullptr;
}
const prism::ModDescriptor kDescriptor{
  sizeof(prism::ModDescriptor), prism::kModApiVersion,
  "prism.example.hello", "Prism Hello Mod", "1.0.0", load, unload
};
}
extern "C" __declspec(dllexport)
const prism::ModDescriptor* __cdecl PrismGetModDescriptor() { return &kDescriptor; }
