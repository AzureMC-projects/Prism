#include "prism/mod_api.h"
#include <cassert>
#include <type_traits>

int main() {
  static_assert(prism::kModApiVersion == 1);
  static_assert(std::is_standard_layout_v<prism::HostApi>);
  static_assert(std::is_standard_layout_v<prism::ModDescriptor>);
  assert(sizeof(prism::ModDescriptor) >= sizeof(std::uint32_t) * 2);
  return 0;
}
