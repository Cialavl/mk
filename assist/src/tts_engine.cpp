#include <assist/tts_engine.h>

#include <assist/utf8_path.h>

#include <tvlib/tts.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

#if defined(_WIN32)
#include <Windows.h>
#include <mmsystem.h>
#endif

namespace mk::assist {
namespace {

namespace fs = std::filesystem;

fs::path default_models_dir() {
    if (const auto env = env_utf8("MK_TTS_MODELS"); env && !env->empty()) {
        return path_from_utf8(*env);
    }
    return path_from_utf8("C:/Users/26495/Desktop/voc/exported_models/longcat_3.5b");
}

fs::path default_voice_dir() {
    if (const auto env = env_utf8("MK_TTS_VOICE"); env && !env->empty()) {
        return path_from_utf8(*env);
    }
    return path_from_utf8("C:/Users/26495/Desktop/voc/audio_text/voices/爱莉希雅_一般_平常");
}

int default_steps() {
    if (const auto env = env_utf8("MK_TTS_STEPS"); env && !env->empty()) {
        try {
            const int value = std::stoi(*env);
            if (value > 0 && value <= 64) return value;
        } catch (const std::exception&) {
        }
    }
    return 4;
}

} // namespace

TtsEngine::TtsEngine(std::filesystem::path models_dir, std::filesystem::path voice_dir, int steps)
    : models_dir_(models_dir.empty() ? default_models_dir() : std::move(models_dir)),
      voice_dir_(voice_dir.empty() ? default_voice_dir() : std::move(voice_dir)),
      steps_(steps > 0 ? steps : default_steps()) {
    if (!fs::exists(models_dir_)) {
        throw std::runtime_error("TTS models not found: " + utf8_from_path(models_dir_));
    }
    if (!fs::exists(voice_dir_)) {
        throw std::runtime_error("TTS voice not found: " + utf8_from_path(voice_dir_));
    }
}

void TtsEngine::speak(const std::string& text) {
    if (text.empty()) return;

    const fs::path wav = working_temp_dir() /
                         ("mk-tts-" + std::to_string(GetCurrentProcessId()) + "-" +
                          std::to_string(GetTickCount64()) + ".wav");

    tvlib::Request request;
    request.models_dir = utf8_from_path(models_dir_);
    request.voice_dir = utf8_from_path(voice_dir_);
    request.text = text;
    request.out_wav = utf8_from_path(wav);
    request.steps = steps_;
    request.guidance = 4.0f;
    request.use_apg = true;

    const tvlib::Result result = tvlib::synthesize(request, tvlib::Events{});
    if (!result.ok) {
        std::error_code ec;
        fs::remove(wav, ec);
        throw std::runtime_error(result.error.empty() ? "TTS failed" : result.error);
    }

    const BOOL played = PlaySoundW(wav.wstring().c_str(), nullptr,
                                   SND_FILENAME | SND_SYNC | SND_NODEFAULT);
    std::error_code ec;
    fs::remove(wav, ec);

    if (!played) {
        throw std::runtime_error("failed to play synthesized audio");
    }
}

} // namespace mk::assist
