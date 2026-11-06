#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <common_command/common_command.h>

namespace AbsStratety {

class CAbsStratry {
public:
    using CommandPtr = std::unique_ptr<CommonCommand::CCommonCommand>;

    virtual ~CAbsStratry() = default;

    virtual int execute(std::vector<std::string> v_command) = 0;
    virtual void initmap() = 0;

    void add(std::string command, CommandPtr command_type);
    [[nodiscard]] const std::map<std::string, CommandPtr>& getmap() const;
    [[nodiscard]] std::vector<std::string> separate(std::string commands) const;

protected:
    std::map<std::string, CommandPtr> name_command;
};

} // namespace AbsStratety
