#pragma once

#include <memory>
#include <string>
#include <vector>

enum class NodeType {
    Directory,
    File
};

struct DirEntry {
    std::string name;
    NodeType type;
    bool executable;
};

class Node {
public:
    std::string name;
    NodeType type;
    Node* parent;
    std::vector<std::unique_ptr<Node>> children;

    explicit Node(std::string nodeName, NodeType nodeType = NodeType::Directory, Node* parentNode = nullptr);
    Node* findChild(const std::string& childName) const;
};

class FileSystem {
private:
    std::unique_ptr<Node> root;
    Node* current;

    std::vector<std::string> splitPath(const std::string& path) const;
    Node* resolvePath(const std::string& path, Node* startNode, std::string* invalidSegment) const;
    bool resolveParentAndName(const std::string& path, Node*& parentOut, std::string& nameOut, std::string* invalidSegment) const;

public:
    FileSystem();

    std::string mkdir(const std::string& dirName);
    std::string mkdirPath(const std::string& path, bool createParents);
    std::string cd(const std::string& target);
    std::string rm(const std::string& target, bool recursive);
    std::string ls() const;
    std::string lsPath(const std::string& path) const;
    std::string pwd() const;
    std::vector<std::string> getCurrentDirectoryNames() const;
    std::vector<std::string> suggestPaths(const std::string& partialPath) const;
    std::vector<DirEntry> listEntries(const std::string& path) const;
};
