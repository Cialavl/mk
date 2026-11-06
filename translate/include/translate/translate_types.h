#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace mk::translate {

struct LangPair {
    std::string src;
    std::string tgt;

    [[nodiscard]] bool empty() const noexcept { return src.empty() || tgt.empty(); }
    [[nodiscard]] std::string code() const;
};

struct TranslateOptions {
    std::size_t max_input_chars{1500};
    int max_new_tokens{2048};
    bool greedy{true};
};

// Accepts legacy "zhen"/"enzh" and generic "zh-en"/"zh_en" forms.
[[nodiscard]] std::optional<LangPair> parse_lang_pair(std::string_view code);

// True when the code is in the built-in language table (zh, en, ja, ...).
[[nodiscard]] bool is_known_lang(std::string_view code);

// Accepts compact command names such as "zhja", "jaen", "enfr".
[[nodiscard]] std::optional<LangPair> parse_compact_pair(std::string_view code);

// Tries compact, legacy and dashed forms in that order.
[[nodiscard]] std::optional<LangPair> parse_pair_code(std::string_view code);

// Chinese display name used by the translation prompt; falls back to the code.
[[nodiscard]] std::string language_name_zh(std::string_view code);

} // namespace mk::translate
