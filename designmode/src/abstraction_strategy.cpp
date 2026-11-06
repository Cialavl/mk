#include <designmode/abstraction_strategy/abstraction_strategy.h>

#include <utility>

namespace AbsStratety {

void CAbsStratry::add(std::string command, CommandPtr command_type) {
    name_command.insert_or_assign(std::move(command), std::move(command_type));
}

const std::map<std::string, CAbsStratry::CommandPtr>& CAbsStratry::getmap() const {
    return name_command;
}

std::vector<std::string> CAbsStratry::separate(std::string commands) const {
    (void)commands;
    return {};
}

} // namespace AbsStratety
