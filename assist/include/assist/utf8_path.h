#pragma once

// std::filesystem on Windows treats narrow strings as the ANSI code page, while
// mk passes UTF-8 everywhere. These helpers convert explicitly.

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace mk::assist {

std::filesystem::path path_from_utf8(std::string_view utf8);
std::string utf8_from_path(const std::filesystem::path& path);

// Reads an environment variable as UTF-8; empty when unset.
std::optional<std::string> env_utf8(const char* name);

// First writable scratch directory: MK_TMP, %LOCALAPPDATA%\mk\tmp, system
// temp, then .mk-tmp in the working directory. Throws when none is usable.
std::filesystem::path working_temp_dir();

} // namespace mk::assist
