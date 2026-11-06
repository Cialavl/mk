#pragma once

#include <common_command/common_command.h>
#include <translate_command/translate_command.h>

#include <memory>
#include <string>

class TranslateCommand : public CommonCommand::CCommonCommand {
public:
    explicit TranslateCommand(std::string type);
    int MkRun() override;
    int MkRun(std::string receive_commands) override;

private:
    int run_generic(const std::string& arguments);

    std::string type_;
    std::unique_ptr<atomizationCmd_translate::AtomCmdTranslate> command_;
};
