#include "main.h"
#include "serve.h"

#include <mk_command_strategy/mk_command_strategy.h>
#include <translate/translate_types.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::vector<std::string> commands{};
    commands.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
    for (int i = 1; i < argc; ++i) {
        commands.emplace_back(mk::app::to_utf8(argv[i]));
    }

    if (commands.empty()) {
        std::cerr << "usage: mk <command> [arguments]\n"
                     "  mk <src><tgt> <text>   e.g. zhen, enzh, zhja, jaen, enfr\n"
                     "  mk tr --to <lang> [--from <lang>] <text>\n"
                     "  mk <src><tgt> <text> --speak    朗读译文\n"
                     "  mk ocr <image> [--from zh] [--to en] [--speak]\n"
                     "  mk say <text>\n"
                     "  mk serve\n";
        return 1;
    }

    try {
        if (commands.front() == "serve") return mk::app::run_serve();
        if (commands.front() == "ocr") return mk::app::run_ocr(commands);
        if (commands.front() == "say") return mk::app::run_say(commands);

        // Translation with --speak needs the translated text before printing, so
        // it bypasses the generic command strategy.
        const bool has_speak = std::find(commands.begin() + 1, commands.end(), "--speak") != commands.end();
        if (has_speak && (commands.front() == "tr" ||
                          mk::translate::parse_pair_code(commands.front()).has_value())) {
            return mk::app::run_translate(commands);
        }

        MkCommandStrategy::CMKCommandStrategy strategy(commands.front());
        strategy.initmap();
        return strategy.execute(commands);
    } catch (const std::exception& error) {
        std::cerr << "mk: " << error.what() << '\n';
        return 1;
    }
}
