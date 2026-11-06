#include <translate/text_chunker.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace mk::translate {
namespace {

constexpr std::size_t kNoPos = std::string_view::npos;

std::size_t cp_width(std::string_view text, std::size_t index) {
    const auto lead = static_cast<unsigned char>(text[index]);
    if ((lead & 0x80U) == 0) return 1;
    if ((lead & 0xE0U) == 0xC0U) return 2;
    if ((lead & 0xF0U) == 0xE0U) return 3;
    if ((lead & 0xF8U) == 0xF0U) return 4;
    return 1;
}

std::size_t advance_cp(std::string_view text, std::size_t pos, std::size_t count) {
    while (count > 0 && pos < text.size()) {
        pos += cp_width(text, pos);
        --count;
    }
    return pos;
}

bool is_sentence_end(std::string_view text, std::size_t index) {
    switch (text[index]) {
    case '.':
    case '!':
    case '?':
    case ';':
    case '\n':
        return true;
    default:
        break;
    }

    const auto rest = text.substr(index);
    for (const auto mark : {std::string_view{"。"}, std::string_view{"！"}, std::string_view{"？"},
                            std::string_view{"；"}, std::string_view{"…"}}) {
        if (rest.starts_with(mark)) return true;
    }
    return false;
}

} // namespace

TextChunker::TextChunker(std::size_t max_chars)
    : max_chars_(max_chars == 0 ? 1 : max_chars) {}

std::vector<std::string> TextChunker::split(std::string_view text) const {
    std::vector<std::string> chunks;
    std::size_t start = 0;

    while (start < text.size()) {
        while (start < text.size() &&
               (text[start] == '\n' || text[start] == '\r' || text[start] == ' ' || text[start] == '\t')) {
            ++start;
        }
        if (start >= text.size()) break;

        const std::size_t limit = advance_cp(text, start, max_chars_);
        if (limit >= text.size()) {
            chunks.emplace_back(text.substr(start));
            break;
        }

        std::size_t cut = kNoPos;
        for (std::size_t i = start; i <= limit && i < text.size();) {
            const std::size_t width = cp_width(text, i);
            if (is_sentence_end(text, i)) cut = i + width;
            i += width;
        }

        if (cut == kNoPos || cut <= start) cut = limit;
        chunks.emplace_back(text.substr(start, cut - start));
        start = cut;
    }

    return chunks;
}

} // namespace mk::translate
