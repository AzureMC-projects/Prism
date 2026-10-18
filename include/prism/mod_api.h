#pragma once
#include <cstdint>

#if defined(_WIN32)
  #if defined(PRISM_RUNTIME_EXPORTS)
    #define PRISM_API __declspec(dllexport)
  #else
    #define PRISM_API __declspec(dllimport)
  #endif
  #define PRISM_CALL __cdecl
#else
  #define PRISM_API
  #define PRISM_CALL
#endif

namespace prism {
inline constexpr std::uint32_t kModApiVersion = 1;
inline constexpr char kDescriptorExport[] = "PrismGetModDescriptor";

struct HostApi {
  std::uint32_t api_version;
  void (PRISM_CALL* log_info)(const char* message);
  void (PRISM_CALL* log_warning)(const char* message);
  void (PRISM_CALL* log_error)(const char* message);
};

struct ModDescriptor {
  std::uint32_t struct_size;
  std::uint32_t api_version;
  const char* id;
  const char* name;
  const char* version;
  bool (PRISM_CALL* on_load)(const HostApi* host);
  void (PRISM_CALL* on_unload)();
};

using GetModDescriptorFn = const ModDescriptor* (PRISM_CALL*)();
} // namespace prism

// Each mod DLL must export:
// extern "C" __declspec(dllexport)
// const prism::ModDescriptor* __cdecl PrismGetModDescriptor();
