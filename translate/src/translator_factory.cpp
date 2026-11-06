#include <translate/translator_factory.h>

#include <translate/llama_translator.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace mk::translate {
namespace fs = std::filesystem;

namespace {

constexpr const char* kFallbackModel = "index-2b/Index-Translate-2B.Q4_K_M.gguf";

bool is_usable(const fs::path& path, std::uintmax_t expected_bytes) {
    std::error_code ec;
    if (!fs::exists(path, ec)) return false;
    if (expected_bytes == 0) return true;
    const auto size = fs::file_size(path, ec);
    return !ec && size >= expected_bytes;
}

} // namespace

TranslatorPtr make_translator(const TranslateConfig& config, const LangPair& pair) {
    if (config.engine != "llama") {
        throw std::runtime_error("unknown translation engine: " + config.engine);
    }

    const auto primary = config.llama_model;
    const auto fallback = config.model_root / kFallbackModel;
    auto resolved = config;

    if (is_usable(primary, config.expected_bytes)) {
        return std::make_unique<LlamaTranslator>(resolved);
    }

    if (primary != fallback && is_usable(fallback, 0)) {
        std::cerr << "[translate] incomplete model: " << primary.string() << "\n"
                  << "[translate] using " << fallback.string() << "\n";
        resolved.llama_model = fallback;
        return std::make_unique<LlamaTranslator>(resolved);
    }

    throw std::runtime_error("model not ready: " + primary.string() +
                             " (place the GGUF under model/index-9b)");
}

} // namespace mk::translate
