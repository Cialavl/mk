#pragma once

#include <filesystem>
#include <string>

namespace mk::assist {

class TtsEngine {
public:
    TtsEngine(std::filesystem::path models_dir = {},
              std::filesystem::path voice_dir = {},
              int steps = 0);  // 0 = MK_TTS_STEPS or 4
    ~TtsEngine() = default;

    TtsEngine(const TtsEngine&) = delete;
    TtsEngine& operator=(const TtsEngine&) = delete;

    // Synthesizes to a temporary WAV, plays it, then deletes the file.
    void speak(const std::string& text);

    [[nodiscard]] const std::filesystem::path& voice_dir() const noexcept { return voice_dir_; }

private:
    std::filesystem::path models_dir_;
    std::filesystem::path voice_dir_;
    int steps_ = 8;
};

} // namespace mk::assist
