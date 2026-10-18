#include "plugin_manager.h"
#include "logger.h"
#include <exception>
#include <set>
#include <string>

namespace prism::detail {
namespace {
void PRISM_CALL host_info(const char* m) { log(LogLevel::info, m ? m : ""); }
void PRISM_CALL host_warning(const char* m) { log(LogLevel::warning, m ? m : ""); }
void PRISM_CALL host_error(const char* m) { log(LogLevel::error, m ? m : ""); }
const HostApi kHost{ kModApiVersion, host_info, host_warning, host_error };
bool valid_text(const char* s) { return s && *s && std::char_traits<char>::length(s) < 256; }
}
PluginManager::~PluginManager() { unload_all(); }

void PluginManager::load_directory(const std::filesystem::path& directory) noexcept {
  try {
    if (!std::filesystem::exists(directory)) {
      std::filesystem::create_directories(directory);
      log(LogLevel::info, "Created mods directory; no mods to load yet.");
      return;
    }
    std::set<std::string> ids;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (!entry.is_regular_file() || entry.path().extension() != L".dll") continue;
      const auto path = std::filesystem::absolute(entry.path());
      HMODULE module = LoadLibraryW(path.c_str());
      if (!module) {
        log(LogLevel::warning, "Skipped a mod DLL that Windows could not load.");
        continue;
      }
      auto get = reinterpret_cast<GetModDescriptorFn>(GetProcAddress(module, kDescriptorExport));
      if (!get) {
        log(LogLevel::warning, "Skipped DLL without PrismGetModDescriptor export.");
        FreeLibrary(module);
        continue;
      }
      const ModDescriptor* d = nullptr;
      try { d = get(); } catch (...) { d = nullptr; }
      if (!d || d->struct_size < sizeof(ModDescriptor) || d->api_version != kModApiVersion ||
          !valid_text(d->id) || !valid_text(d->name) || !valid_text(d->version) ||
          !d->on_load || !d->on_unload) {
        log(LogLevel::warning, "Skipped mod with invalid descriptor or incompatible API.");
        FreeLibrary(module);
        continue;
      }
      if (!ids.insert(d->id).second) {
        log(LogLevel::warning, "Skipped duplicate mod ID.");
        FreeLibrary(module);
        continue;
      }
      bool ok = false;
      try { ok = d->on_load(&kHost); } catch (...) { ok = false; }
      if (!ok) {
        log(LogLevel::warning, "Mod rejected initialization.");
        FreeLibrary(module);
        continue;
      }
      loaded_.push_back({module, d, d->id});
      log(LogLevel::info, std::string("Loaded mod: ") + d->id + " (" + d->version + ")");
    }
  } catch (const std::exception& e) {
    log(LogLevel::error, std::string("Mod scan failed: ") + e.what());
  } catch (...) {
    log(LogLevel::error, "Mod scan failed with an unknown error.");
  }
}

void PluginManager::unload_all() noexcept {
  for (auto it = loaded_.rbegin(); it != loaded_.rend(); ++it) {
    try { it->descriptor->on_unload(); }
    catch (...) { log(LogLevel::error, "Mod threw during unload."); }
    if (it->module) FreeLibrary(it->module);
  }
  loaded_.clear();
}
}
