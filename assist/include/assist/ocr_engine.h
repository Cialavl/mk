#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace mk::assist {

class OcrEngine {
public:
    explicit OcrEngine(std::filesystem::path paddle_root = {});
    ~OcrEngine();

    OcrEngine(const OcrEngine&) = delete;
    OcrEngine& operator=(const OcrEngine&) = delete;

    // Returns recognized text in reading order (top-to-bottom, left-to-right).
    std::string recognize(const std::filesystem::path& image);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mk::assist
