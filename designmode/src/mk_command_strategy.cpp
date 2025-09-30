#define WIN32_LEAN_AND_MEAN
#include<commands/translate_command.h>
#include<mk_command_strategy/mk_command_strategy.h>
#include<commands/gcc_command.h>
#include <commands/add_command.h>
#include <commands/new_command.h>
#include <boost/locale/encoding.hpp>
int MkCommandStrategy::CMKCommandStrategy::execute(std::vector<std::string> v_command)
{

	if (1 == v_command.size()) {
		getmap()[v_command.at(0)]->MkRun();
		return 0;
	}
	else if (1 < v_command.size()) {
		std::string cmd{ v_command.at(1)};
		
        for (size_t i = 3 ; i <= v_command.size();  ++i)
		{
			cmd = cmd+" "+ v_command.at(i - 1);
		}
		//cmd = cmd + v_command.at(i);
		if(!isUtf8(cmd)) {
       
			cmd = boost::locale::conv::between(cmd, "UTF-8", "GBK");
		}
		//std::cout << "cmd:" << cmd << std::endl;
		getmap()[v_command.at(0)]->MkRun(cmd);
		return 0;
	}
	else
	{
		std::cout << "vector is null" << std::endl;
		return 0;
	}
}
void MkCommandStrategy::CMKCommandStrategy::initmap()
{
	//add("cmake", new CmakeCommand);
	if (cmdtype == "new") {

		add("new", new NewCommand);
	}
	else if (cmdtype == "add")
	{
		add("add", new AddCommand);
	}
	else if (cmdtype == "grun")
	{
		add("grun", new GccCommandRun);
	}
	else if(cmdtype == "zhen"){
		add("zhen", new TranslateCommand("zhen"));
	}
	else if(cmdtype == "enzh")
	{
		add("enzh", new TranslateCommand("enzh"));
	}
	

}
