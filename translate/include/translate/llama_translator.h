#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <translate/i_translator.h>
#include <translate/translate_config.h>

namespace mk::translate {

class LlamaTranslator final : public ITranslator {
public:
    explicit LlamaTranslator(const TranslateConfig& config);
    ~LlamaTranslator() override;

    LlamaTranslator(const LlamaTranslator&) = delete;
    LlamaTranslator& operator=(const LlamaTranslator&) = delete;

    [[nodiscard]] std::string translate(std::string_view text,
                                        const LangPair& pair,
                                        const TranslateOptions& options) override;

    [[nodiscard]] std::string_view name() const noexcept override { return "llama.cpp"; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mk::translate
