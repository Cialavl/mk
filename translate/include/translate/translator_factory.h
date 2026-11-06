#pragma once

#include <translate/i_translator.h>
#include <translate/translate_config.h>

namespace mk::translate {

// Throws std::runtime_error when the backend cannot be created.
[[nodiscard]] TranslatorPtr make_translator(const TranslateConfig& config, const LangPair& pair);

} // namespace mk::translate
