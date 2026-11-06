#include <translate/translate_types.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <utility>

namespace mk::translate {
namespace {

std::string to_lower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

constexpr std::array<std::pair<std::string_view, std::string_view>, 35> kNames{{
    {"zh", "中文"}, {"en", "英文"}, {"ja", "日语"}, {"ko", "韩语"}, {"fr", "法语"},
    {"de", "德语"}, {"es", "西班牙语"}, {"ru", "俄语"}, {"pt", "葡萄牙语"}, {"it", "意大利语"},
    {"ar", "阿拉伯语"}, {"th", "泰语"}, {"vi", "越南语"}, {"id", "印尼语"}, {"hi", "印地语"},
    {"tr", "土耳其语"}, {"nl", "荷兰语"}, {"pl", "波兰语"}, {"cs", "捷克语"}, {"uk", "乌克兰语"},
    {"fa", "波斯语"}, {"he", "希伯来语"}, {"bn", "孟加拉语"}, {"ta", "泰米尔语"}, {"te", "泰卢固语"},
    {"mr", "马拉地语"}, {"gu", "古吉拉特语"}, {"ur", "乌尔都语"}, {"kk", "哈萨克语"}, {"mn", "蒙古语"},
    {"bo", "藏语"}, {"ms", "马来语"}, {"tl", "菲律宾语"}, {"km", "高棉语"}, {"my", "缅甸语"},
}};

bool is_lang_code(std::string_view s) {
    return s.size() >= 2 && s.size() <= 8 &&
           std::ranges::all_of(s, [](unsigned char c) { return std::isalpha(c) != 0; });
}

} // namespace

std::string LangPair::code() const {
    return src + "-" + tgt;
}

std::optional<LangPair> parse_lang_pair(std::string_view code) {
    const auto low = to_lower(code);
    if (low == "zhen") return LangPair{"zh", "en"};
    if (low == "enzh") return LangPair{"en", "zh"};

    const auto sep = low.find_first_of("-_");
    if (sep == std::string::npos || sep == 0 || sep + 1 >= low.size()) return std::nullopt;

    auto src = std::string_view(low).substr(0, sep);
    auto tgt = std::string_view(low).substr(sep + 1);
    if (!is_lang_code(src) || !is_lang_code(tgt)) return std::nullopt;

    return LangPair{std::string(src), std::string(tgt)};
}

bool is_known_lang(std::string_view code) {
    const auto low = to_lower(code);
    return std::ranges::any_of(kNames, [&](const auto& item) { return item.first == low; });
}

std::optional<LangPair> parse_compact_pair(std::string_view code) {
    const auto low = to_lower(code);
    for (std::size_t split = 2; split + 2 <= low.size(); ++split) {
        const auto src = std::string_view(low).substr(0, split);
        const auto tgt = std::string_view(low).substr(split);
        if (is_known_lang(src) && is_known_lang(tgt)) {
            return LangPair{std::string(src), std::string(tgt)};
        }
    }
    return std::nullopt;
}

std::optional<LangPair> parse_pair_code(std::string_view code) {
    if (auto pair = parse_compact_pair(code)) return pair;
    return parse_lang_pair(code);
}

std::string language_name_zh(std::string_view code) {
    const auto low = to_lower(code);
    for (const auto& [key, name] : kNames) {
        if (key == low) return std::string(name);
    }
    return low;
}

} // namespace mk::translate