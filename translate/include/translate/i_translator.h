#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <translate/translate_types.h>

namespace mk::translate {

class ITranslator {
public:
    virtual ~ITranslator() = default;

    [[nodiscard]] virtual std::string translate(std::string_view text,
                                                const LangPair& pair,
                                                const TranslateOptions& options) = 0;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

using TranslatorPtr = std::unique_ptr<ITranslator>;

} // namespace mk::translate
