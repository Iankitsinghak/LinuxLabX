#pragma once

#include <string>

namespace logging {

void init(const std::string& appName, const std::string& version);
void info(const std::string& message);
void error(const std::string& message);
std::string getLogFilePath();

}  // namespace logging
