#define WIN32_LEAN_AND_MEAN

#include <commands/translate_command.h>
#include <mk_command_strategy/mk_command_strategy.h>
#include <commands/gcc_command.h>
#include <commands/add_command.h>
#include <commands/new_command.h>

#include <boost/locale/encoding.hpp>
#include <translate/translate_types.h>

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int MkCommandStrategy::CMKCommandStrategy::execute(std::vector<std::string> v_command) {
    if (v_command.empty()) {
        std::cerr << "no command given\n";
        return 1;
    }

    const auto& commands = getmap();
    const auto it = commands.find(v_command.front());
    if (it == commands.end()) {
        std::cerr << "unknown command: " << v_command.front() << '\n';
        return 1;
    }

    auto& command = *it->second;
    if (v_command.size() == 1) {
        return command.MkRun();
    }

    std::string text = v_command.at(1);
    for (std::size_t i = 2; i < v_command.size(); ++i) {
        text += ' ';
        text += v_command.at(i);
    }

    if (!isUtf8(text)) {
        text = boost::locale::conv::between(text, "UTF-8", "GBK");
    }

    return command.MkRun(text);
}

void MkCommandStrategy::CMKCommandStrategy::initmap() {
    if (cmdtype == "new") {
        add("new", std::make_unique<NewCommand>());
    } else if (cmdtype == "add") {
        add("add", std::make_unique<AddCommand>());
    } else if (cmdtype == "grun") {
        add("grun", std::make_unique<GccCommandRun>());
    } else if (cmdtype == "zhen") {
        add("zhen", std::make_unique<TranslateCommand>("zhen"));
    } else if (cmdtype == "enzh") {
        add("enzh", std::make_unique<TranslateCommand>("enzh"));
    } else if (cmdtype == "tr") {
        add("tr", std::make_unique<TranslateCommand>("tr"));
    } else if (mk::translate::parse_pair_code(cmdtype).has_value()) {
        add(cmdtype, std::make_unique<TranslateCommand>(cmdtype));
    }
}
