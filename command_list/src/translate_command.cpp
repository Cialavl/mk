#define WIN32_LEAN_AND_MEAN

#include <translate_command/translate_command.h>

#include <translate/translator_factory.h>

#include <boost/locale/encoding.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <clocale>
#include <langinfo.h>
#include <unistd.h>
#endif

namespace {

bool is_terminal() {
#if defined(_WIN32) || defined(_WIN64)
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr) return false;
    if (GetFileType(handle) != FILE_TYPE_CHAR) return false;
    DWORD mode = 0;
    return GetConsoleMode(handle, &mode) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

std::string terminal_encoding() {
#if defined(_WIN32) || defined(_WIN64)
    switch (GetConsoleOutputCP()) {
    case 65001: return "UTF-8";
    case 936: return "GBK";
    case 950: return "BIG5";
    case 932: return "SHIFT-JIS";
    case 949: return "EUC-KR";
    default: return "GBK";
    }
#else
    const std::string encoding = nl_langinfo(CODESET);
    return encoding.empty() ? "UTF-8" : encoding;
#endif
}

} // namespace

namespace atomizationCmd_translate {

AtomCmdTranslate::AtomCmdTranslate(std::string type)
    : type_(std::move(type)), chunker_(1500) {
    const auto parsed = mk::translate::parse_pair_code(type_);
    if (!parsed) throw std::invalid_argument("unsupported language pair: " + type_);
    pair_ = *parsed;
    load_config();
}

AtomCmdTranslate::AtomCmdTranslate(mk::translate::LangPair pair, std::string engine,
                                   std::filesystem::path model)
    : pair_(std::move(pair)), chunker_(1500) {
    if (pair_.src.empty() || pair_.tgt.empty()) {
        throw std::invalid_argument("source and target languages are required");
    }

    load_config();
    if (!engine.empty()) config_.engine = std::move(engine);
    if (!model.empty()) {
        config_.llama_model = std::filesystem::absolute(std::move(model));
        config_.expected_bytes = 0;  // user supplied file: trust its size
    }
}

AtomCmdTranslate::~AtomCmdTranslate() = default;

void AtomCmdTranslate::load_config() {
    const auto root = mk::translate::find_model_root();
    const auto loaded = mk::translate::TranslateConfig::load(root / "config.json");
    config_ = loaded ? *loaded : mk::translate::TranslateConfig::defaults(root);
    config_.model_root = root;

    options_.max_new_tokens = config_.max_new_tokens;
    chunker_ = mk::translate::TextChunker(static_cast<std::size_t>(options_.max_input_chars));
}

void AtomCmdTranslate::ensure_loaded() {
    if (!translator_) {
        translator_ = mk::translate::make_translator(config_, pair_);
    }
}

void AtomCmdTranslate::translation(const std::string& input) {
    if (input.empty()) {
        output({});
        return;
    }

    ensure_loaded();

    std::vector<std::string> lines;
    const auto chunks = chunker_.split(input);
    lines.reserve(chunks.size());
    for (const auto& chunk : chunks) {
        lines.push_back(translator_->translate(chunk, pair_, options_));
    }
    output(std::move(lines));
}

void AtomCmdTranslate::output(std::vector<std::string>&& lines) {
    const auto encoding = is_terminal() ? terminal_encoding() : std::string{"UTF-8"};

    for (auto& line : lines) {
        if (encoding != "UTF-8") {
            line = boost::locale::conv::between(line, encoding, "UTF-8");
        }
        std::cout << line << '\n';
    }
}

} // namespace atomizationCmd_translate
