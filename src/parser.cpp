#include "parser.h"

#include <sstream>

Command parseInput(const std::string& input) {
    Command cmd;
    std::istringstream iss(input);
    std::string token;

    if (!(iss >> cmd.name)) {
        return cmd;
    }

    while (iss >> token) {
        cmd.args.push_back(token);
    }

    return cmd;
}
