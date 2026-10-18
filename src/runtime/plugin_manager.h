#pragma once
#include <Windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include "prism/mod_api.h"

namespace prism::detail {
class PluginManager {
public:
  PluginManager() = default;
  ~PluginManager();
  PluginManager(const PluginManager&) = delete;
  PluginManager& operator=(const PluginManager&) = delete;
  void load_directory(const std::filesystem::path& directory) noexcept;
  void unload_all() noexcept;
private:
  struct LoadedMod {
    HMODULE module{};
    const ModDescriptor* descriptor{};
    std::string id;
  };
  std::vector<LoadedMod> loaded_;
};
}
