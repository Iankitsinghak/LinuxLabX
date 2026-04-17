#pragma once

#include <string>
#include <vector>

#include "command.h"
#include "filesystem.h"

std::vector<std::string> getSuggestions(const std::string& input, FileSystem& fs);
std::string getSuggestion(const std::string& lastCommand, const std::vector<std::string>& args);
bool wasSuccessfulExecution(const Command& cmd, const std::string& output);
