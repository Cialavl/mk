#pragma once

#include <string>
#include <vector>

namespace mk::app {

int run_serve();
int run_ocr(const std::vector<std::string>& args);
int run_say(const std::vector<std::string>& args);
int run_translate(const std::vector<std::string>& args);

// Windows argv / console input arrive in the ANSI code page; normalise to UTF-8.
std::string to_utf8(std::string text);

} // namespace mk::app
