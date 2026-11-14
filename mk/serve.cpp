#include "serve.h"

#include <assist/ocr_engine.h>
#include <assist/tts_engine.h>

#include <translate/i_translator.h>
#include <translate/text_chunker.h>
#include <translate/translate_config.h>
#include <translate/translate_types.h>
#include <translate/translator_factory.h>

#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mk::app {
namespace {

using mk::translate::LangPair;
using mk::translate::TextChunker;
using mk::translate::TranslateConfig;
using mk::translate::TranslateOptions;
using mk::translate::TranslatorPtr;

std::string trim(std::string value) {
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ')) {
        value.pop_back();
    }
    std::size_t begin = 0;
    while (begin < value.size() && value[begin] == ' ') ++begin;
    return value.substr(begin);
}

std::string join(const std::vector<std::string>& args, std::size_t first) {
    std::string result;
    for (std::size_t i = first; i < args.size(); ++i) {
        if (!result.empty()) result.push_back(' ');
        result += args[i];
    }
    return result;
}

// One llama model serves every language pair, so the loaded translator is
// shared across calls instead of being rebuilt per pair.
class SharedTranslator {
public:
    std::string run(const LangPair& pair, const std::string& text) {
        if (text.empty()) return {};
        ensure(pair);

        const auto chunks = chunker_.split(text);
        std::string result;
        for (const auto& chunk : chunks) {
            if (!result.empty()) result.push_back('\n');
            result += translator_->translate(chunk, pair, options_);
        }
        return result;
    }

private:
    void ensure(const LangPair& pair) {
        if (translator_) return;

        const auto root = mk::translate::find_model_root();
        const auto loaded = TranslateConfig::load(root / "config.json");
        config_ = loaded ? *loaded : TranslateConfig::defaults(root);
        config_.model_root = root;

        options_.max_new_tokens = config_.max_new_tokens;
        chunker_ = TextChunker(static_cast<std::size_t>(options_.max_input_chars));
        translator_ = mk::translate::make_translator(config_, pair);
    }

    TranslateConfig config_;
    TranslateOptions options_;
    TranslatorPtr translator_;
    TextChunker chunker_{1500};
};

struct Session {
    SharedTranslator translator;
    std::unique_ptr<mk::assist::OcrEngine> ocr;
    std::unique_ptr<mk::assist::TtsEngine> tts;

    void speak(const std::string& text) {
        if (!tts) tts = std::make_unique<mk::assist::TtsEngine>();
        tts->speak(text);
    }
};

struct OcrRequest {
    std::string image;
    std::string from = "zh";
    std::string to;  // empty = recognition only
    bool speak = false;
};

OcrRequest parse_ocr(const std::vector<std::string>& args, std::size_t first) {
    OcrRequest request;
    for (std::size_t i = first; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--to" && i + 1 < args.size()) {
            request.to = args[++i];
        } else if (arg == "--from" && i + 1 < args.size()) {
            request.from = args[++i];
        } else if (arg == "--speak") {
            request.speak = true;
        } else if (request.image.empty()) {
            request.image = arg;
        } else {
            throw std::runtime_error("unexpected argument: " + arg);
        }
    }
    if (request.image.empty()) {
        throw std::runtime_error("ocr requires an image path");
    }
    return request;
}

void run_ocr_request(Session& session, const OcrRequest& request) {
    if (!session.ocr) session.ocr = std::make_unique<mk::assist::OcrEngine>();

    const std::string text = session.ocr->recognize(request.image);
    std::cout << text << '\n' << std::flush;

    if (request.to.empty()) {
        if (request.speak) session.speak(text);
        return;
    }

    const std::string translated = session.translator.run({request.from, request.to}, text);
    std::cout << translated << '\n' << std::flush;
    if (request.speak) session.speak(translated);
}

} // namespace

int run_ocr(const std::vector<std::string>& args) {
    Session session;
    run_ocr_request(session, parse_ocr(args, 1));
    return 0;
}

int run_say(const std::vector<std::string>& args) {
    const std::string text = join(args, 1);
    if (text.empty()) throw std::runtime_error("say requires text");

    Session session;
    session.speak(text);
    return 0;
}

int run_serve() {
    Session session;

    std::cout << "mk serve: <src><tgt> <text> | ocr <image> [--from zh] [--to en] [--speak] | say <text> | quit\n";
    std::cout << std::flush;

    std::string line;
    while (std::getline(std::cin, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;
        if (line == "help") {
            std::cout << "<src><tgt> <text> | ocr <image> [--from zh] [--to en] [--speak] | say <text> | quit\n";
            std::cout << std::flush;
            continue;
        }

        try {
            const auto space = line.find(' ');
            const std::string command = line.substr(0, space);
            std::string rest = space == std::string::npos ? std::string{} : line.substr(space + 1);

            if (command == "ocr") {
                std::vector<std::string> args{"ocr"};
                std::istringstream stream(rest);
                for (std::string token; stream >> token;) args.push_back(token);
                run_ocr_request(session, parse_ocr(args, 1));
                continue;
            }

            if (command == "say") {
                session.speak(rest);
                continue;
            }

            bool speak_result = false;
            constexpr std::string_view kSpeakSuffix = " --speak";
            if (rest.size() >= kSpeakSuffix.size() &&
                rest.compare(rest.size() - kSpeakSuffix.size(), kSpeakSuffix.size(), kSpeakSuffix) == 0) {
                rest.resize(rest.size() - kSpeakSuffix.size());
                speak_result = true;
            }

            const auto pair = mk::translate::parse_pair_code(command);
            if (!pair) {
                std::cerr << "error: unknown command: " << command << '\n';
                continue;
            }

            const std::string result = session.translator.run(*pair, rest);
            std::cout << result << '\n' << std::flush;

            if (speak_result) session.speak(result);
        } catch (const std::exception& error) {
            std::cerr << "error: " << error.what() << '\n';
        }
    }
    return 0;
}

} // namespace mk::app
