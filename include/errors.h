#pragma once

#include <string>
#include <vector>

int levenshteinDistance(const std::string& a, const std::string& b);
std::string getClosestCommandSuggestion(const std::string& inputCommand, const std::vector<std::string>& validCommands);
std::string suggestCorrection(const std::string& inputCommand, const std::vector<std::string>& validCommands);
std::string getErrorMessage(const std::string& command, const std::string& errorType);
