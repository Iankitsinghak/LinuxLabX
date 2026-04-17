#include "filesystem.h"

#include <algorithm>
#include <utility>

#include "command_registry.h"
#include "errors.h"

Node::Node(std::string nodeName, NodeType nodeType, Node* parentNode)
    : name(std::move(nodeName)), type(nodeType), parent(parentNode) {}

Node* Node::findChild(const std::string& childName) const {
    for (const auto& child : children) {
        if (child->name == childName) {
            return child.get();
        }
    }
    return nullptr;
}

FileSystem::FileSystem() {
    root = std::make_unique<Node>("/", NodeType::Directory, nullptr);

    auto home = std::make_unique<Node>("home", NodeType::Directory, root.get());
    Node* homePtr = home.get();

    auto user = std::make_unique<Node>("user", NodeType::Directory, homePtr);
    Node* userPtr = user.get();

    auto docs = std::make_unique<Node>("docs", NodeType::Directory, userPtr);
    docs->children.push_back(std::make_unique<Node>("notes.txt", NodeType::File, docs.get()));
    user->children.push_back(std::move(docs));
    user->children.push_back(std::make_unique<Node>("Downloads", NodeType::Directory, userPtr));
    user->children.push_back(std::make_unique<Node>("script.sh", NodeType::File, userPtr));

    home->children.push_back(std::move(user));
    root->children.push_back(std::move(home));

    current = userPtr;
}

std::vector<std::string> FileSystem::splitPath(const std::string& path) const {
    std::vector<std::string> segments;
    std::string currentPart;

    for (char c : path) {
        if (c == '/') {
            if (!currentPart.empty()) {
                segments.push_back(currentPart);
                currentPart.clear();
            }
        } else {
            currentPart.push_back(c);
        }
    }

    if (!currentPart.empty()) {
        segments.push_back(currentPart);
    }

    return segments;
}

Node* FileSystem::resolvePath(const std::string& path, Node* startNode, std::string* invalidSegment) const {
    Node* node = startNode;
    std::vector<std::string> segments = splitPath(path);

    for (const std::string& segment : segments) {
        if (segment == ".") {
            continue;
        }
        if (segment == "..") {
            if (node->parent != nullptr) {
                node = node->parent;
            }
            continue;
        }

        Node* child = node->findChild(segment);
        if (child == nullptr) {
            if (invalidSegment != nullptr) {
                *invalidSegment = segment;
            }
            return nullptr;
        }
        if (child->type != NodeType::Directory) {
            if (invalidSegment != nullptr) {
                *invalidSegment = segment;
            }
            return nullptr;
        }
        node = child;
    }

    return node;
}

bool FileSystem::resolveParentAndName(const std::string& path, Node*& parentOut, std::string& nameOut, std::string* invalidSegment) const {
    if (path.empty()) {
        return false;
    }

    std::string trimmed = path;
    while (trimmed.size() > 1 && trimmed.back() == '/') {
        trimmed.pop_back();
    }

    if (trimmed == "/") {
        return false;
    }

    std::size_t slashPos = trimmed.find_last_of('/');
    std::string parentPath;

    if (slashPos == std::string::npos) {
        parentOut = current;
        nameOut = trimmed;
        return !nameOut.empty();
    }

    if (slashPos == 0) {
        parentPath = "/";
    } else {
        parentPath = trimmed.substr(0, slashPos);
    }

    nameOut = trimmed.substr(slashPos + 1);
    if (nameOut.empty() || nameOut == "." || nameOut == "..") {
        return false;
    }

    Node* start = (parentPath.empty() || parentPath[0] != '/') ? current : root.get();
    parentOut = resolvePath(parentPath, start, invalidSegment);
    return parentOut != nullptr;
}

std::string FileSystem::mkdir(const std::string& dirName) {
    return mkdirPath(dirName, false);
}

std::string FileSystem::mkdirPath(const std::string& path, bool createParents) {
    if (path.empty()) {
        return getErrorMessage("mkdir", "missing_argument");
    }

    std::string trimmed = path;
    while (trimmed.size() > 1 && trimmed.back() == '/') {
        trimmed.pop_back();
    }

    if (trimmed == "/") {
        return "mkdir: invalid directory name";
    }

    Node* node = (trimmed[0] == '/') ? root.get() : current;
    std::vector<std::string> segments = splitPath(trimmed);
    bool createdAny = false;

    for (size_t i = 0; i < segments.size(); ++i) {
        const std::string& segment = segments[i];
        bool isLast = (i + 1 == segments.size());

        if (segment == ".") {
            continue;
        }
        if (segment == "..") {
            if (node->parent != nullptr) {
                node = node->parent;
            }
            continue;
        }

        Node* child = node->findChild(segment);
        if (child != nullptr) {
            if (isLast && !createParents) {
                return getErrorMessage("mkdir", "duplicate_directory");
            }
            if (child->type != NodeType::Directory) {
                return "Invalid path segment: '" + segment + "'";
            }
            node = child;
            continue;
        }

        if (!createParents && !isLast) {
            return "Invalid path segment: '" + segment + "'";
        }

        node->children.push_back(std::make_unique<Node>(segment, NodeType::Directory, node));
        node = node->children.back().get();
        createdAny = true;
    }

    if (createdAny) {
        return "Directory created: " + segments.back();
    }

    return "Directory already exists: " + segments.back();
}

std::string FileSystem::cd(const std::string& target) {
    if (target.empty()) {
        return getErrorMessage("cd", "missing_argument");
    }

    Node* start = (target[0] == '/') ? root.get() : current;
    std::string invalidSegment;
    Node* resolved = resolvePath(target, start, &invalidSegment);
    if (resolved == nullptr) {
        return "Invalid path segment: '" + invalidSegment + "'";
    }

    current = resolved;
    return "Changed directory to: " + pwd();
}

std::string FileSystem::rm(const std::string& target, bool recursive) {
    if (target.empty()) {
        return getErrorMessage("rm", "missing_argument");
    }
    if (target == "/") {
        return "Cannot delete root directory";
    }

    Node* parent = nullptr;
    std::string name;
    std::string invalidSegment;
    if (!resolveParentAndName(target, parent, name, &invalidSegment)) {
        if (!invalidSegment.empty()) {
            return "Invalid path segment: '" + invalidSegment + "'";
        }
        return "rm: invalid target";
    }

    if (parent == nullptr) {
        return "rm: invalid target";
    }

    auto buildPath = [this](Node* node) {
        if (node == root.get()) {
            return std::string("/");
        }
        std::vector<std::string> parts;
        Node* currentNode = node;
        while (currentNode != nullptr && currentNode != root.get()) {
            parts.push_back(currentNode->name);
            currentNode = currentNode->parent;
        }
        std::string path;
        for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
            path += "/" + *it;
        }
        return path;
    };

    auto isProtected = [](const std::string& path) {
        return path == "/" || path == "/home" || path == "/home/user";
    };

    for (size_t i = 0; i < parent->children.size(); ++i) {
        if (parent->children[i]->name != name) {
            continue;
        }

        std::string fullPath = buildPath(parent->children[i].get());
        if (isProtected(fullPath)) {
            return "Protected path. Deletion not allowed.";
        }

        if (parent->children[i]->type == NodeType::Directory && !recursive && !parent->children[i]->children.empty()) {
            return "Use rm -r to delete directories";
        }

        parent->children.erase(parent->children.begin() + static_cast<long>(i));
        return "Directory removed: " + name;
    }

    return "Directory not found.";
}

std::string FileSystem::ls() const {
    return lsPath("");
}

std::string FileSystem::lsPath(const std::string& path) const {
    Node* node = current;
    if (!path.empty()) {
        Node* start = (path[0] == '/') ? root.get() : current;
        std::string invalidSegment;
        node = resolvePath(path, start, &invalidSegment);
        if (node == nullptr) {
            return "Invalid path segment: '" + invalidSegment + "'";
        }
    }

    if (node->children.empty()) {
        return "(empty)";
    }

    std::string output;
    for (size_t i = 0; i < node->children.size(); ++i) {
        output += node->children[i]->name;
        if (i + 1 < node->children.size()) {
            output += "  ";
        }
    }
    return output;
}

std::string FileSystem::pwd() const {
    if (current == root.get()) {
        return "/";
    }

    std::vector<std::string> parts;
    Node* node = current;
    while (node != nullptr && node != root.get()) {
        parts.push_back(node->name);
        node = node->parent;
    }

    std::string path;
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        path += "/" + *it;
    }
    return path;
}

std::vector<std::string> FileSystem::getCurrentDirectoryNames() const {
    std::vector<std::string> names;
    names.reserve(current->children.size());
    for (const auto& child : current->children) {
        if (child->type == NodeType::Directory) {
            names.push_back(child->name);
        }
    }
    return names;
}

std::vector<std::string> FileSystem::suggestPaths(const std::string& partialPath) const {
    std::vector<std::string> suggestions;

    std::string raw = partialPath;
    bool absolute = !raw.empty() && raw[0] == '/';
    bool endsWithSlash = !raw.empty() && raw.back() == '/';

    std::string dirPart;
    std::string namePrefix;
    std::size_t slashPos = raw.find_last_of('/');
    if (slashPos == std::string::npos) {
        dirPart = "";
        namePrefix = raw;
    } else {
        dirPart = raw.substr(0, slashPos + 1);
        namePrefix = endsWithSlash ? "" : raw.substr(slashPos + 1);
    }

    Node* base = nullptr;
    if (dirPart.empty()) {
        base = absolute ? root.get() : current;
    } else {
        std::string invalidSegment;
        Node* start = (dirPart[0] == '/') ? root.get() : current;
        base = resolvePath(dirPart, start, &invalidSegment);
        if (base == nullptr) {
            return suggestions;
        }
    }

    if (base == nullptr) {
        return suggestions;
    }

    std::string prefix = dirPart;
    if (prefix.empty() && absolute) {
        prefix = "/";
    }

    for (const auto& child : base->children) {
        if (child->type != NodeType::Directory) {
            continue;
        }
        if (!startsWith(child->name, namePrefix)) {
            continue;
        }
        suggestions.push_back(prefix + child->name);
    }

    std::sort(suggestions.begin(), suggestions.end());
    return suggestions;
}

std::vector<DirEntry> FileSystem::listEntries(const std::string& path) const {
    Node* node = current;
    if (!path.empty()) {
        Node* start = (path[0] == '/') ? root.get() : current;
        std::string invalidSegment;
        node = resolvePath(path, start, &invalidSegment);
        if (node == nullptr) {
            return {};
        }
    }

    std::vector<DirEntry> entries;
    entries.reserve(node->children.size());
    for (const auto& child : node->children) {
        bool executable = (child->type == NodeType::File && child->name.size() >= 3 && child->name.rfind(".sh") == child->name.size() - 3);
        entries.push_back({child->name, child->type, executable});
    }
    return entries;
}
