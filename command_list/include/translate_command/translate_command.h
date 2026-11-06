#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <translate/i_translator.h>
#include <translate/text_chunker.h>
#include <translate/translate_config.h>

namespace atomizationCmd_translate {

class AtomCmdTranslate {
public:
    explicit AtomCmdTranslate(std::string type);

    // Generic entry: pair plus optional engine/model overrides.
    AtomCmdTranslate(mk::translate::LangPair pair,
                     std::string engine = {},
                     std::filesystem::path model = {});

    ~AtomCmdTranslate();

    AtomCmdTranslate(const AtomCmdTranslate&) = delete;
    AtomCmdTranslate& operator=(const AtomCmdTranslate&) = delete;

    void translation(const std::string& input);
    void output(std::vector<std::string>&& lines);

private:
    void load_config();
    void ensure_loaded();

    std::string type_;
    mk::translate::LangPair pair_;
    mk::translate::TranslateConfig config_;
    mk::translate::TranslateOptions options_;
    mk::translate::TranslatorPtr translator_;
    mk::translate::TextChunker chunker_;
};

} // namespace atomizationCmd_translate
