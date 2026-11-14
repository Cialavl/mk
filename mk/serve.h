#pragma once

#include <string>
#include <vector>

namespace mk::app {

int run_serve();
int run_ocr(const std::vector<std::string>& args);
int run_say(const std::vector<std::string>& args);

} // namespace mk::app
