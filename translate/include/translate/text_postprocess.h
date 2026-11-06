#pragma once

#include <string>

namespace mk::translate {

// Trims, drops prompt echoes/thinking blocks and normalizes line endings.
[[nodiscard]] std::string clean_translation(std::string text);

} // namespace mk::translate
