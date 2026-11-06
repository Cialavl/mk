#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mk::translate {

// Splits UTF-8 text on sentence boundaries without cutting code points.
class TextChunker {
public:
    explicit TextChunker(std::size_t max_chars = 1500);

    [[nodiscard]] std::vector<std::string> split(std::string_view text) const;
    [[nodiscard]] std::size_t max_chars() const noexcept { return max_chars_; }

private:
    std::size_t max_chars_;
};

} // namespace mk::translate
