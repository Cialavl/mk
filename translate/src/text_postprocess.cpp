#include <translate/text_postprocess.h>

#include <algorithm>
#include <cstring>
#include <string_view>

namespace mk::translate {
namespace {

std::string_view trim_view(std::string_view text) {
    const auto is_space = [](char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    };
    while (!text.empty() && is_space(text.front())) text.remove_prefix(1);
    while (!text.empty() && is_space(text.back())) text.remove_suffix(1);
    return text;
}

} // namespace

std::string clean_translation(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());

    if (text.starts_with("\xEF\xBB\xBF")) {
        text.erase(0, 3);
    }

    if (const auto begin = text.find("<think>"); begin != std::string::npos) {
        if (const auto end = text.find("</think>", begin); end != std::string::npos) {
            text.erase(begin, end + 8 - begin);
        } else {
            text.erase(begin);
        }
    }

    for (const std::string_view label : {"译文：", "译文:", "翻译：", "翻译:", "Translation:"}) {
        if (text.starts_with(label)) {
            text.erase(0, label.size());
            break;
        }
    }

    const auto trimmed = trim_view(text);
    std::string out;
    out.reserve(trimmed.size());

    int newlines = 0;
    for (const char c : trimmed) {
        if (c == '\n') {
            if (newlines < 2) out.push_back(c);
            ++newlines;
        } else {
            newlines = 0;
            out.push_back(c);
        }
    }
    return out;
}

} // namespace mk::translate
