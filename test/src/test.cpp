
#include <abstraction_command/abstraction_command.h>
#include <commands/cmake_command.h>
#include <mk_command_strategy/mk_command_strategy.h>

int main(int argc, char* argv[]) {
    std::vector<std::string> v_commands{};
    //if (argc < 2) {
    //    printf("error\n");
    //    return 0;
    //}
 
        v_commands.push_back("enzh");
        v_commands.push_back("hello");
    
    MkCommandStrategy::CMKCommandStrategy Strategy(v_commands.at(0));
    Strategy.initmap();
    Strategy.execute(v_commands);
    return 0;
}
