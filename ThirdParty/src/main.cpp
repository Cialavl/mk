#include <ThirdParty/thirdparty.h>
std::string stringCharacterReplace(std::string str, char _oldch, std::string _newch) {
    std::string res{};
    for (auto ch : str) {
        if (ch == _oldch) {
            res = res + _newch;
        } else {
            res = res + ch;
        }
    }
    return res;
}
int run(const char* cmd) {

    int result = system(cmd);

    if (result == 0) {

        std::cout << "CMD命令执行成功." << std::endl;
    } else {

        std::cerr << "CMD命令执行失败." << std::endl;
    }

    return 0;
}

//std::string exec_cmd(const char* cmd) {
//    std::string line;
//    try {
//        bp::ipstream pipe_stream; // 用于捕获输出
//        bp::child c(cmd, bp::std_out > pipe_stream); // 在 Unix/Linux 上
//
//        while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()) {
//            std::cout << line << std::endl; // 输出命令结果
//        }
//
//        c.wait(); // 等待进程结束
//        std::cout << "Command executed successfully." << std::endl;
//    } catch (const std::exception& e) {
//        std::cerr << "Error: " << e.what() << std::endl;
//    }
//    return line;
//}
void TextTurnsSound(std::string _txt) {
    std::string cmd_string = " echo '" + _txt
                           + "' | .\\piper\\piper.exe --model .\\piper\\zh_CN-huayan-medium.onnx -c "
                             ".\\piper\\zh_zh_CN_huayan_medium_zh_CN-huayan-medium.onnx.json "
                             "--output_file ./b.wav";
    run(cmd_string.c_str());
}
int create_txt(std::string _txtPath, std::string _nameTxt) {
    std::ofstream oFile;
    // 不存在则新建文件
    oFile.open(_txtPath, std::ios::app);
    if (!oFile) { // true则说明文件打开出错
        std::cout << "error 1" << std::endl;
    } else {
        oFile << _nameTxt;
    }

    oFile.close();

    return 0;
}
