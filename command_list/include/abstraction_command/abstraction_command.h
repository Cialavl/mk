#pragma once
#include<string>
namespace AbsCommand {
    class Abs_Command
	{
	public:
		int run_cmd(const char* cmd);
	private:
		const char* cmd;
	};
}