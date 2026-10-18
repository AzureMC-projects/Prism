#pragma once
#include <filesystem>

namespace prism::detail {
void start_overlay(const std::filesystem::path& install_root,
                   const std::filesystem::path& data_root) noexcept;
}
