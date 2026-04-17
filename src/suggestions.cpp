#include "suggestions.h"

#include <sstream>

#include "command_registry.h"

std::vector<std::string> getSuggestions(const std::string& input, FileSystem& fs) {
    const std::vector<std::string>& validCommands = getValidCommands();
    std::vector<std::string> suggestions;

    if (input.empty()) {
        return validCommands;
    }

    bool endsWithSpace = !input.empty() && input.back() == ' ';
    std::istringstream iss(input);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return validCommands;
    }

    if (tokens.size() == 1 && !endsWithSpace) {
        const std::string& commandPrefix = tokens[0];
        for (const std::string& command : validCommands) {
            if (startsWith(command, commandPrefix)) {
                suggestions.push_back(command);
            }
        }
        return suggestions;
    }

    const std::string& command = tokens[0];
    if (command != "cd" && command != "mkdir") {
        return suggestions;
    }

    std::string dirPrefix;
    if (tokens.size() >= 2) {
        dirPrefix = endsWithSpace ? "" : tokens.back();
    } else if (!endsWithSpace) {
        return suggestions;
    }

    std::vector<std::string> paths = fs.suggestPaths(dirPrefix);
    if (command == "mkdir") {
        for (std::string& entry : paths) {
            if (entry.empty() || entry.back() == '/') {
                continue;
            }
            entry += "/";
        }
    }
    return paths;
}

std::string getSuggestion(const std::string& lastCommand, const std::vector<std::string>& args) {
    if (lastCommand == "mkdir" && !args.empty()) {
        std::string target;
        for (const std::string& arg : args) {
            if (arg == "-p") {
                continue;
            }
            target = arg;
        }
        if (!target.empty()) {
            return "You can use 'cd " + target + "' to enter the directory";
        }
    }

    if (lastCommand == "ls") {
        return "You can navigate using 'cd <folder>'";
    }

    if (lastCommand == "cd" && args.size() == 1) {
        return "Use 'ls' to view contents";
    }

    return "";
}

bool wasSuccessfulExecution(const Command& cmd, const std::string& output) {
    if (cmd.name == "help" || cmd.name == "tutorial") {
        return !output.empty();
    }

    if (cmd.name == "mkdir") {
        return startsWith(output, "Directory created:") || startsWith(output, "Directory already exists:");
    }

    if (cmd.name == "cd") {
        return startsWith(output, "Changed directory to:");
    }

    if (cmd.name == "ls") {
        return !startsWith(output, "ls: usage:");
    }

    return false;
}
