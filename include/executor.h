#pragma once

#include <string>

#include "command.h"
#include "filesystem.h"

class Executor {
private:
    FileSystem& fs;

public:
    explicit Executor(FileSystem& fileSystem);
    std::string execute(const Command& cmd);
};
