#include "errors.h"

#include <algorithm>
#include <limits>

#include "command_registry.h"

int levenshteinDistance(const std::string& a, const std::string& b) {
    if (a == b) {
        return 0;
    }
    if (a.empty()) {
        return static_cast<int>(b.size());
    }
    if (b.empty()) {
        return static_cast<int>(a.size());
    }

    std::vector<int> prev(b.size() + 1), curr(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j) {
        prev[j] = static_cast<int>(j);
    }

    for (size_t i = 1; i <= a.size(); ++i) {
        curr[0] = static_cast<int>(i);
        for (size_t j = 1; j <= b.size(); ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({
                prev[j] + 1,
                curr[j - 1] + 1,
                prev[j - 1] + cost
            });
        }
        prev.swap(curr);
    }

    return prev[b.size()];
}

std::string getClosestCommandSuggestion(const std::string& inputCommand, const std::vector<std::string>& validCommands) {
    if (inputCommand.empty() || validCommands.empty()) {
        return "";
    }

    int bestDistance = std::numeric_limits<int>::max();
    std::string bestMatch;

    for (const std::string& command : validCommands) {
        int distance = levenshteinDistance(inputCommand, command);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestMatch = command;
        }
    }

    int maxAllowedDistance = (inputCommand.size() <= 5) ? 2 : 3;
    if (bestDistance <= maxAllowedDistance) {
        return bestMatch;
    }

    return "";
}

std::string suggestCorrection(const std::string& inputCommand, const std::vector<std::string>& validCommands) {
    std::string suggestion = getClosestCommandSuggestion(inputCommand, validCommands);
    if (suggestion.empty()) {
        return "";
    }
    return "Did you mean: " + suggestion + "?";
}

std::string getErrorMessage(const std::string& command, const std::string& errorType) {
    if (errorType == "directory_not_found") {
        return "Directory not found. Use 'ls' to check available folders.";
    }

    if (errorType == "missing_argument") {
        if (command == "mkdir") {
            return "Missing argument. Usage: mkdir <folder_name>";
        }
        if (command == "cd") {
            return "Missing argument. Usage: cd <folder_name>";
        }
        if (command == "rm") {
            return "Missing argument. Usage: rm <folder_name>";
        }
        return "Missing argument.";
    }

    if (errorType == "duplicate_directory") {
        return "Directory already exists. Choose a different folder name.";
    }

    if (errorType == "invalid_command") {
        std::string suggestion = getClosestCommandSuggestion(command, getValidCommands());
        if (!suggestion.empty()) {
            return "Invalid command. Did you mean '" + suggestion + "'?";
        }
        return "Invalid command.";
    }

    return "An unknown error occurred.";
}
