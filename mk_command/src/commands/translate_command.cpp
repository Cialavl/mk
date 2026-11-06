#include <commands/translate_command.h>

#include <cctype>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kUsage =
    "usage:\n"
    "  mk <src><tgt> <text>     e.g. zhen, enzh, zhja, jaen, enfr\n"
    "  mk tr --to <lang> [--from <lang>] [--engine llama|ct2] [--model <gguf>] <text>\n";

std::vector<std::string> split_words(const std::string& text) {
    std::vector<std::string> words;
    std::string current;
    for (const char c : text) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            if (!current.empty()) {
                words.push_back(std::move(current));
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) words.push_back(std::move(current));
    return words;
}

} // namespace

TranslateCommand::TranslateCommand(std::string type) : type_(std::move(type)) {}

int TranslateCommand::MkRun() {
    std::cout << kUsage;
    return 0;
}

int TranslateCommand::MkRun(std::string receive_commands) {
    try {
        if (type_ == "tr") {
            return run_generic(receive_commands);
        }

        if (!command_) {
            command_ = std::make_unique<atomizationCmd_translate::AtomCmdTranslate>(type_);
        }
        command_->translation(receive_commands);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "translate error: " << error.what() << '\n';
        return 1;
    }
}

int TranslateCommand::run_generic(const std::string& arguments) {
    const auto words = split_words(arguments);
    if (words.empty()) {
        std::cout << kUsage;
        return 0;
    }

    std::string from;
    std::string to;
    std::string engine;
    std::string model;
    std::string text;

    for (std::size_t i = 0; i < words.size(); ++i) {
        const auto& word = words[i];
        if (word == "--help" || word == "-h") {
            std::cout << kUsage;
            return 0;
        }
        if (word == "--from" || word == "--to" || word == "--engine" || word == "--model") {
            if (i + 1 >= words.size()) {
                std::cerr << "missing value for " << word << '\n';
                return 1;
            }
            const auto& value = words[++i];
            if (word == "--from") from = value;
            else if (word == "--to") to = value;
            else if (word == "--engine") engine = value;
            else model = value;
            continue;
        }
        if (!text.empty()) text.push_back(' ');
        text += word;
    }

    if (to.empty()) {
        std::cerr << "mk tr: --to is required\n" << kUsage;
        return 1;
    }
    if (text.empty()) {
        std::cerr << "mk tr: no text to translate\n";
        return 1;
    }

    auto command = std::make_unique<atomizationCmd_translate::AtomCmdTranslate>(
        mk::translate::LangPair{from.empty() ? "auto" : from, to}, engine, model);
    command->translation(text);
    return 0;
}
