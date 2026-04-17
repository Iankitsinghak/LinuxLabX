#include "executor.h"

#include "errors.h"

namespace {
std::string buildHelpText() {
    return
        "LinuxLabX beginner help\n"
        "Commands:\n"
        "  help                Show this help screen\n"
        "  tutorial            Guided 5-step mini practice\n"
        "  pwd                 Show current folder\n"
        "  ls [path]           List contents\n"
        "  cd <folder>|..      Change folder\n"
        "  mkdir [-p] <name>   Create folder\n"
        "  rm [-r] [-f] <name> Remove file/folder\n"
        "  exit                Close the terminal\n"
        "\n"
        "Tip: type 'suggest <partial>' to see command/path suggestions.";
}

std::string buildTutorialText() {
    return
        "Mini tutorial (copy and run these one by one):\n"
        "  1) pwd\n"
        "  2) mkdir practice\n"
        "  3) cd practice\n"
        "  4) mkdir notes\n"
        "  5) ls\n"
        "\n"
        "Optional cleanup:\n"
        "  cd ..\n"
        "  rm -r practice";
}
}

Executor::Executor(FileSystem& fileSystem) : fs(fileSystem) {}

std::string Executor::execute(const Command& cmd) {
    if (cmd.name.empty()) {
        return "";
    }

    if (cmd.name == "help") {
        if (!cmd.args.empty()) {
            return "Usage: help";
        }
        return buildHelpText();
    }

    if (cmd.name == "tutorial") {
        if (!cmd.args.empty()) {
            return "Usage: tutorial";
        }
        return buildTutorialText();
    }

    if (cmd.name == "mkdir") {
        if (cmd.args.empty()) {
            return getErrorMessage("mkdir", "missing_argument");
        }

        bool createParents = false;
        std::string target;

        for (const std::string& arg : cmd.args) {
            if (arg == "-p") {
                createParents = true;
                continue;
            }
            if (!target.empty()) {
                return "Usage: mkdir [-p] <folder_name>";
            }
            target = arg;
        }

        if (target.empty()) {
            return "Usage: mkdir [-p] <folder_name>";
        }

        return fs.mkdirPath(target, createParents);
    }

    if (cmd.name == "cd") {
        if (cmd.args.empty()) {
            return getErrorMessage("cd", "missing_argument");
        }
        if (cmd.args.size() != 1) {
            return "Usage: cd <folder_name>";
        }
        return fs.cd(cmd.args[0]);
    }

    if (cmd.name == "ls") {
        if (cmd.args.size() > 1) {
            return "Usage: ls [path]";
        }
        if (cmd.args.empty()) {
            return fs.ls();
        }
        return fs.lsPath(cmd.args[0]);
    }

    if (cmd.name == "rm") {
        if (cmd.args.empty()) {
            return getErrorMessage("rm", "missing_argument");
        }

        bool recursive = false;
        bool force = false;
        std::string target;

        for (const std::string& arg : cmd.args) {
            if (!arg.empty() && arg[0] == '-') {
                if (arg.find('r') != std::string::npos) {
                    recursive = true;
                }
                if (arg.find('f') != std::string::npos) {
                    force = true;
                }
                continue;
            }

            if (!target.empty()) {
                return "Usage: rm [-r] [-f] <folder_name>";
            }
            target = arg;
        }

        if (target.empty()) {
            return recursive ? "Usage: rm -r <folder_name>" : "Usage: rm <folder_name>";
        }

        (void)force;
        return fs.rm(target, recursive);
    }

    return getErrorMessage(cmd.name, "invalid_command");
}
