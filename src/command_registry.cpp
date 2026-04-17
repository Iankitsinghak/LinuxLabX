#include "command_registry.h"

bool startsWith(const std::string& value, const std::string& prefix) {
    if (prefix.size() > value.size()) {
        return false;
    }
    return value.compare(0, prefix.size(), prefix) == 0;
}

const std::vector<std::string>& getValidCommands() {
    static const std::vector<std::string> validCommands = {
        "mkdir", "cd", "ls", "rm", "pwd", "help", "tutorial", "exit"
    };
    return validCommands;
}
