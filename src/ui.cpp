#include "ui.h"

#if defined(LINUXLABX_HAS_QT_WIDGETS)
#define HAS_QT_WIDGETS LINUXLABX_HAS_QT_WIDGETS
#elif __has_include(<QApplication>)
#define HAS_QT_WIDGETS 1
#else
#define HAS_QT_WIDGETS 0
#endif

#if HAS_QT_WIDGETS
#include <QApplication>
#include <QFont>
#include <QDesktopServices>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTimer>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#else
#include <iostream>
#endif

#include <functional>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <future>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "command.h"
#include "command_registry.h"
#include "executor.h"
#include "filesystem.h"
#include "parser.h"
#include "suggestions.h"
#include "version.h"

#if HAS_QT_WIDGETS
namespace {
bool isTokenChar(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-' || ch == '/';
}

std::string extractTokenAt(const std::string& text, int index) {
    if (text.empty() || index < 0 || index >= static_cast<int>(text.size())) {
        return "";
    }
    if (!isTokenChar(text[index])) {
        return "";
    }

    int left = index;
    while (left > 0 && isTokenChar(text[left - 1])) {
        --left;
    }

    int right = index;
    while (right + 1 < static_cast<int>(text.size()) && isTokenChar(text[right + 1])) {
        ++right;
    }

    return text.substr(static_cast<size_t>(left), static_cast<size_t>(right - left + 1));
}

QString appDataDirPath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/.linuxlabx";
    }

    QDir d(dir);
    d.mkpath(".");
    return d.absolutePath();
}

QString appStateFilePath() {
    return appDataDirPath() + "/app_state.json";
}

QString usageFilePath() {
    return appDataDirPath() + "/usage.json";
}

QJsonObject readJsonObject(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return QJsonObject();
    }
    return doc.object();
}

void writeJsonObject(const QString& filePath, const QJsonObject& object) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
}

bool isFirstLaunch() {
    QJsonObject state = readJsonObject(appStateFilePath());
    return !state.value("launched_before").toBool(false);
}

void markLaunchState() {
    QJsonObject state = readJsonObject(appStateFilePath());
    if (!state.contains("first_launch_at")) {
        state["first_launch_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }
    state["launched_before"] = true;
    state["last_launch_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    writeJsonObject(appStateFilePath(), state);
}

QString osName() {
#if defined(Q_OS_MACOS)
    return "macOS";
#elif defined(Q_OS_WIN)
    return "Windows";
#elif defined(Q_OS_LINUX)
    return "Linux";
#else
    return "Unknown";
#endif
}

void updateUsageStats() {
    QJsonObject usage = readJsonObject(usageFilePath());
    int launchCount = usage.value("launch_count").toInt(0);
    usage["launch_count"] = launchCount + 1;
    usage["os"] = osName();
    usage["last_launch_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    writeJsonObject(usageFilePath(), usage);
}

std::vector<int> parseVersionNumbers(const std::string& version) {
    std::string normalized = version;
    if (!normalized.empty() && normalized[0] == 'v') {
        normalized.erase(0, 1);
    }

    std::vector<int> numbers;
    std::stringstream ss(normalized);
    std::string segment;
    while (std::getline(ss, segment, '.')) {
        int value = 0;
        for (char ch : segment) {
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                break;
            }
            value = value * 10 + (ch - '0');
        }
        numbers.push_back(value);
    }
    while (numbers.size() < 3) {
        numbers.push_back(0);
    }
    return numbers;
}

bool isRemoteVersionNewer(const std::string& currentVersion, const std::string& remoteVersion) {
    std::vector<int> curr = parseVersionNumbers(currentVersion);
    std::vector<int> remote = parseVersionNumbers(remoteVersion);
    for (size_t i = 0; i < 3; ++i) {
        if (remote[i] > curr[i]) {
            return true;
        }
        if (remote[i] < curr[i]) {
            return false;
        }
    }
    return false;
}

struct UpdateInfo {
    bool hasUpdate = false;
    QString version;
    QString releaseUrl;
};

UpdateInfo checkForUpdate(const std::string& currentVersion) {
    UpdateInfo result;
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("https://api.github.com/repos/Iankitsinghak/LinuxLabX/releases/latest"));
    request.setRawHeader("User-Agent", "LinuxLabX");

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(3000);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
    }

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return result;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return result;
    }

    QJsonObject obj = doc.object();
    QString tag = obj.value("tag_name").toString();
    QString htmlUrl = obj.value("html_url").toString();
    if (tag.isEmpty() || htmlUrl.isEmpty()) {
        return result;
    }

    if (isRemoteVersionNewer(currentVersion, tag.toStdString())) {
        result.hasUpdate = true;
        result.version = tag;
        result.releaseUrl = htmlUrl;
    }
    return result;
}
}

class TerminalOutputArea : public QPlainTextEdit {
private:
    const std::unordered_map<std::string, std::string>* commandHelp;

public:
    explicit TerminalOutputArea(QWidget* parent = nullptr)
        : QPlainTextEdit(parent), commandHelp(nullptr) {
        setMouseTracking(true);
        viewport()->setMouseTracking(true);
    }

    void setCommandHelp(const std::unordered_map<std::string, std::string>* helpMap) {
        commandHelp = helpMap;
    }

protected:
    void mouseMoveEvent(QMouseEvent* event) override {
        if (event == nullptr || commandHelp == nullptr) {
            QPlainTextEdit::mouseMoveEvent(event);
            return;
        }

        QTextCursor cursor = cursorForPosition(event->pos());
        std::string line = cursor.block().text().toStdString();
        int idx = cursor.positionInBlock();
        if (idx >= static_cast<int>(line.size())) {
            idx = static_cast<int>(line.size()) - 1;
        }

        std::string word = extractTokenAt(line, idx);

        auto it = commandHelp->find(word);
        if (it != commandHelp->end()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QToolTip::showText(event->globalPosition().toPoint(), QString::fromStdString(it->second), this);
#else
            QToolTip::showText(event->globalPos(), QString::fromStdString(it->second), this);
#endif
        } else {
            QToolTip::hideText();
        }

        QPlainTextEdit::mouseMoveEvent(event);
    }
};

class TerminalInputField : public QLineEdit {
private:
    std::function<void()> tabHandler;
    std::function<void()> upHandler;
    std::function<void()> downHandler;
    const std::unordered_map<std::string, std::string>* commandHelp;

public:
    explicit TerminalInputField(QWidget* parent = nullptr)
        : QLineEdit(parent), commandHelp(nullptr) {
        setMouseTracking(true);
    }

    void setCommandHelp(const std::unordered_map<std::string, std::string>* helpMap) {
        commandHelp = helpMap;
    }

    void setTabHandler(std::function<void()> handler) {
        tabHandler = std::move(handler);
    }

    void setUpHandler(std::function<void()> handler) {
        upHandler = std::move(handler);
    }

    void setDownHandler(std::function<void()> handler) {
        downHandler = std::move(handler);
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event != nullptr && event->key() == Qt::Key_Tab) {
            if (tabHandler) {
                tabHandler();
            }
            event->accept();
            return;
        }

        if (event != nullptr && event->key() == Qt::Key_Up) {
            if (upHandler) {
                upHandler();
            }
            event->accept();
            return;
        }

        if (event != nullptr && event->key() == Qt::Key_Down) {
            if (downHandler) {
                downHandler();
            }
            event->accept();
            return;
        }

        QLineEdit::keyPressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (event == nullptr || commandHelp == nullptr) {
            QLineEdit::mouseMoveEvent(event);
            return;
        }

        int idx = cursorPositionAt(event->pos());
        if (idx >= text().size()) {
            idx = text().size() - 1;
        }

        std::string word = extractTokenAt(text().toStdString(), idx);
        auto it = commandHelp->find(word);
        if (it != commandHelp->end()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QToolTip::showText(event->globalPosition().toPoint(), QString::fromStdString(it->second), this);
#else
            QToolTip::showText(event->globalPos(), QString::fromStdString(it->second), this);
#endif
        } else {
            QToolTip::hideText();
        }

        QLineEdit::mouseMoveEvent(event);
    }
};

class TerminalWindow : public QWidget {
private:
    TerminalOutputArea* outputArea;
    TerminalInputField* inputField;
    FileSystem fs;
    Executor executor;
    std::vector<std::string> commandHistory;
    size_t historyIndex;
    std::string draftInput;
    std::unordered_map<std::string, std::string> commandHelp;
    std::vector<std::string> tabSuggestions;
    size_t tabIndex;
    std::string tabSeed;
    bool awaitingDeleteConfirm;
    std::string pendingDeleteTarget;
    bool pendingDeleteRecursive;
    QTimer* deleteTimer;

    enum class OutputType {
        Normal,
        Success,
        Error,
        Info
    };

public:
    TerminalWindow()
        : executor(fs),
          historyIndex(0),
          tabIndex(0),
          awaitingDeleteConfirm(false),
                    pendingDeleteRecursive(false),
                    deleteTimer(new QTimer(this)) {
        commandHelp = {
            {"mkdir", "Create a directory"},
            {"cd", "Change directory"},
            {"ls", "List directory contents"},
            {"pwd", "Print working directory"},
            {"rm", "Remove files or directories"},
            {"-r", "Recursive delete"},
            {"-f", "Force delete (skip confirmation)"},
            {"-p", "Create parent directories"},
            {"help", "Show beginner-friendly command guide"},
            {"tutorial", "Show a 5-step guided practice"},
            {"exit", "exit: close the terminal"},
            {"suggest", "suggest: show auto-complete matches"}
        };

        setWindowTitle("Linux-like Terminal (Qt)");
        resize(780, 500);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(10);

        outputArea = new TerminalOutputArea(this);
        outputArea->setReadOnly(true);
        outputArea->setLineWrapMode(QPlainTextEdit::NoWrap);
        outputArea->setUndoRedoEnabled(false);
        outputArea->setCommandHelp(&commandHelp);

        inputField = new TerminalInputField(this);
        inputField->setPlaceholderText("Enter command...");
        inputField->setCommandHelp(&commandHelp);

        QFont mono("Consolas");
        mono.setStyleHint(QFont::Monospace);
        mono.setPointSize(11);
        outputArea->setFont(mono);
        inputField->setFont(mono);

        setStyleSheet(
            "QWidget { background-color: #000000; color: #B8FFB8; }"
            "QPlainTextEdit {"
            "  background-color: #000000;"
            "  color: #B8FFB8;"
            "  border: 1px solid #1E1E1E;"
            "  border-radius: 6px;"
            "  padding: 10px;"
            "  selection-background-color: #1E3A1E;"
            "}"
            "QLineEdit {"
            "  background-color: #050505;"
            "  color: #E8FFE8;"
            "  border: 1px solid #1E1E1E;"
            "  border-radius: 6px;"
            "  padding: 10px;"
            "}"
        );

        layout->addWidget(outputArea);
        layout->addWidget(inputField);

        appendOutput("Virtual Linux-like File System");
        appendOutput("Welcome beginner! Type 'help' for a friendly command guide.", OutputType::Info);
        appendOutput("Then try 'tutorial' for a 5-step practice run.", OutputType::Info);
        appendOutput("Commands: mkdir <name>, cd <name>, cd .., ls, rm, pwd, help, tutorial, exit");
        appendOutput("");

        connect(inputField, &QLineEdit::returnPressed, this, [this]() {
            handleCommand();
        });

        connect(inputField, &QLineEdit::textEdited, this, [this](const QString&) {
            tabSuggestions.clear();
            tabIndex = 0;
            tabSeed.clear();
        });

        inputField->setTabHandler([this]() {
            handleTabAutoComplete();
        });
        inputField->setUpHandler([this]() {
            navigateHistoryUp();
        });
        inputField->setDownHandler([this]() {
            navigateHistoryDown();
        });

        deleteTimer->setSingleShot(true);
        connect(deleteTimer, &QTimer::timeout, this, [this]() {
            if (!awaitingDeleteConfirm) {
                return;
            }
            awaitingDeleteConfirm = false;
            pendingDeleteTarget.clear();
            pendingDeleteRecursive = false;
            appendOutput("Delete timed out.", OutputType::Info);
            appendOutput("", OutputType::Info);
        });
    }

    void runStartupCommand(const std::string& command) {
        inputField->setText(QString::fromStdString(command));
        handleCommand();
    }

private:
    std::string promptString() const {
        std::string path = fs.pwd();
        std::string displayPath;
        const std::string homePrefix = "/home/user";

        if (path == homePrefix) {
            displayPath = "~";
        } else if (startsWith(path, homePrefix + "/")) {
            displayPath = "~" + path.substr(homePrefix.size());
        } else {
            displayPath = path;
        }

        return "user@linuxlabx:" + displayPath + "$";
    }

    void appendOutput(const std::string& text, OutputType type = OutputType::Normal) {
        QTextCharFormat format;
        switch (type) {
            case OutputType::Success:
                format.setForeground(QColor("#7CFF7C"));
                break;
            case OutputType::Error:
                format.setForeground(QColor("#FF6B6B"));
                break;
            case OutputType::Info:
                format.setForeground(QColor("#66D9EF"));
                break;
            case OutputType::Normal:
            default:
                format.setForeground(QColor("#E8FFE8"));
                break;
        }

        QTextCursor cursor = outputArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(QString::fromStdString(text + "\n"), format);
        outputArea->setTextCursor(cursor);
        QScrollBar* bar = outputArea->verticalScrollBar();
        if (bar != nullptr) {
            bar->setValue(bar->maximum());
        }
    }

    void appendLsEntries(const std::vector<DirEntry>& entries) {
        QTextCursor cursor = outputArea->textCursor();
        cursor.movePosition(QTextCursor::End);

        QTextCharFormat dirFormat;
        dirFormat.setForeground(QColor("#4FC3F7"));
        QTextCharFormat fileFormat;
        fileFormat.setForeground(QColor("#E8FFE8"));
        QTextCharFormat execFormat;
        execFormat.setForeground(QColor("#7CFF7C"));

        for (size_t i = 0; i < entries.size(); ++i) {
            const DirEntry& entry = entries[i];
            if (entry.type == NodeType::Directory) {
                cursor.insertText(QString::fromStdString(entry.name), dirFormat);
            } else if (entry.executable) {
                cursor.insertText(QString::fromStdString(entry.name), execFormat);
            } else {
                cursor.insertText(QString::fromStdString(entry.name), fileFormat);
            }

            if (i + 1 < entries.size()) {
                cursor.insertText("  ", fileFormat);
            }
        }

        cursor.insertText("\n", fileFormat);
        outputArea->setTextCursor(cursor);

        QScrollBar* bar = outputArea->verticalScrollBar();
        if (bar != nullptr) {
            bar->setValue(bar->maximum());
        }
    }

    void handleTabAutoComplete() {
        std::string currentInput = inputField->text().toStdString();
        if (currentInput != tabSeed) {
            tabSuggestions = getSuggestions(currentInput, fs);
            tabIndex = 0;
            tabSeed = currentInput;
        }

        if (tabSuggestions.empty()) {
            return;
        }

        const std::string& bestMatch = tabSuggestions[tabIndex];
        bool endsWithSpace = !currentInput.empty() && currentInput.back() == ' ';

        std::istringstream iss(currentInput);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }

        std::string completedInput;
        if (tokens.empty()) {
            completedInput = bestMatch;
        } else if (tokens.size() == 1 && !endsWithSpace) {
            completedInput = bestMatch;
        } else if (tokens[0] == "cd" || tokens[0] == "mkdir") {
            completedInput = tokens[0] + " " + bestMatch;
        } else {
            return;
        }

        inputField->setText(QString::fromStdString(completedInput));
        inputField->setCursorPosition(inputField->text().size());

        tabIndex = (tabIndex + 1) % tabSuggestions.size();
    }

    bool parseRmCommand(const Command& cmd, bool& recursive, bool& force, std::string& target) {
        if (cmd.name != "rm") {
            return false;
        }

        recursive = false;
        force = false;
        target.clear();

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
                return false;
            }
            target = arg;
        }

        return !target.empty();
    }

    void navigateHistoryUp() {
        if (commandHistory.empty()) {
            return;
        }

        if (historyIndex == commandHistory.size()) {
            draftInput = inputField->text().toStdString();
        }

        if (historyIndex > 0) {
            --historyIndex;
        }

        inputField->setText(QString::fromStdString(commandHistory[historyIndex]));
        inputField->setCursorPosition(inputField->text().size());
    }

    void navigateHistoryDown() {
        if (commandHistory.empty()) {
            return;
        }

        if (historyIndex < commandHistory.size() - 1) {
            ++historyIndex;
            inputField->setText(QString::fromStdString(commandHistory[historyIndex]));
        } else {
            historyIndex = commandHistory.size();
            inputField->setText(QString::fromStdString(draftInput));
        }

        inputField->setCursorPosition(inputField->text().size());
    }

    void handleCommand() {
        std::string input = inputField->text().toStdString();
        inputField->clear();

        if (input.empty()) {
            return;
        }

        if (awaitingDeleteConfirm) {
            std::string response = input;
            std::transform(response.begin(), response.end(), response.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });

            if (response == "y" || response == "yes") {
                Command rmCmd;
                rmCmd.name = "rm";
                rmCmd.args = {"-r", pendingDeleteTarget};
                std::string output = executor.execute(rmCmd);
                appendOutput(output);
            } else {
                appendOutput("Delete canceled.");
            }

            awaitingDeleteConfirm = false;
            pendingDeleteTarget.clear();
            pendingDeleteRecursive = false;
            appendOutput("");
            return;
        }

        commandHistory.push_back(input);
        historyIndex = commandHistory.size();
        draftInput.clear();

        appendOutput(promptString() + " " + input);

        if (startsWith(input, "suggest ")) {
            std::string partial = input.substr(8);
            std::vector<std::string> suggestions = getSuggestions(partial, fs);

            if (suggestions.empty()) {
                appendOutput("(no suggestions)");
            } else {
                std::string joined;
                for (size_t i = 0; i < suggestions.size(); ++i) {
                    joined += suggestions[i];
                    if (i + 1 < suggestions.size()) {
                        joined += "  ";
                    }
                }
                appendOutput(joined);
            }
            appendOutput("");
            return;
        }

        Command cmd = parseInput(input);
        if (cmd.name.empty()) {
            return;
        }

        if (cmd.name == "exit") {
            appendOutput("Closing terminal window...");
            close();
            return;
        }

        if (cmd.name == "pwd") {
            if (!cmd.args.empty()) {
                appendOutput("Usage: pwd");
                appendOutput("");
                return;
            }
                std::string suggestion = getSuggestion(cmd.name, cmd.args);
                if (!suggestion.empty()) {
                    appendOutput(suggestion, OutputType::Info);
                }
            appendOutput(fs.pwd());
            appendOutput("");
            return;
        }

        bool recursive = false;
            std::string suggestion = getSuggestion(cmd.name, cmd.args);
            if (!suggestion.empty()) {
                appendOutput(suggestion, OutputType::Info);
            }
        bool force = false;
        std::string target;
        if (parseRmCommand(cmd, recursive, force, target) && recursive && !force) {
            awaitingDeleteConfirm = true;
            pendingDeleteTarget = target;
            pendingDeleteRecursive = true;
            appendOutput("Are you sure? (y/n)");
            appendOutput("");
            return;
        }

        std::string output = executor.execute(cmd);
        appendOutput(output);

        if (wasSuccessfulExecution(cmd, output)) {
            std::string suggestion = getSuggestion(cmd.name, cmd.args);
            if (!suggestion.empty()) {
                appendOutput(suggestion);
            }
        }

        appendOutput("");
    }
};
#endif

int runTerminalApp(int argc, char* argv[]) {
#if HAS_QT_WIDGETS
    QApplication app(argc, argv);
    bool firstLaunch = isFirstLaunch();
    updateUsageStats();
    markLaunchState();

    TerminalWindow window;
    window.show();

    if (firstLaunch) {
        QMessageBox welcomeBox(&window);
        welcomeBox.setIcon(QMessageBox::Information);
        welcomeBox.setWindowTitle("Welcome to LinuxLabX");
        welcomeBox.setText("Welcome to LinuxLabX");
        welcomeBox.setInformativeText(
            "Start your learning journey.\n\n"
            "Try your first command: ls"
        );

        QPushButton* startTutorial = welcomeBox.addButton("Start Tutorial", QMessageBox::AcceptRole);
        QPushButton* openTerminal = welcomeBox.addButton("Open Terminal", QMessageBox::ActionRole);
        QPushButton* helpButton = welcomeBox.addButton("Help", QMessageBox::HelpRole);

        welcomeBox.exec();

        if (welcomeBox.clickedButton() == startTutorial) {
            window.runStartupCommand("tutorial");
        } else if (welcomeBox.clickedButton() == helpButton) {
            window.runStartupCommand("help");
        } else if (welcomeBox.clickedButton() == openTerminal) {
            window.runStartupCommand("help");
            window.runStartupCommand("ls");
        }
    }

    UpdateInfo update = checkForUpdate(LINUXLABX_VERSION_STRING);
    if (update.hasUpdate) {
        QMessageBox updateBox(&window);
        updateBox.setIcon(QMessageBox::Information);
        updateBox.setWindowTitle("LinuxLabX Update");
        updateBox.setText("New version available");
        updateBox.setInformativeText("Version " + update.version + " is available.");

        QPushButton* downloadButton = updateBox.addButton("Download update", QMessageBox::AcceptRole);
        updateBox.addButton("Later", QMessageBox::RejectRole);
        updateBox.exec();

        if (updateBox.clickedButton() == downloadButton) {
            QDesktopServices::openUrl(QUrl(update.releaseUrl));
        }
    }

    return app.exec();
#else
    (void)argc;
    (void)argv;

    FileSystem fs;
    Executor executor(fs);
    std::string input;
    bool awaitingDeleteConfirm = false;
    std::string pendingDeleteTarget;

    enum class OutputType {
        Normal,
        Success,
        Error,
        Info
    };

    auto printOutput = [](const std::string& text, OutputType type) {
        const char* color = "\033[0m";
        switch (type) {
            case OutputType::Success:
                color = "\033[32m";
                break;
            case OutputType::Error:
                color = "\033[31m";
                break;
            case OutputType::Info:
                color = "\033[36m";
                break;
            case OutputType::Normal:
            default:
                color = "\033[0m";
                break;
        }
        std::cout << color << text << "\033[0m" << "\n";
    };

    auto printLsEntries = [](const std::vector<DirEntry>& entries) {
        const char* dirColor = "\033[34m";
        const char* execColor = "\033[32m";
        const char* fileColor = "\033[37m";

        for (size_t i = 0; i < entries.size(); ++i) {
            const DirEntry& entry = entries[i];
            const char* color = fileColor;
            if (entry.type == NodeType::Directory) {
                color = dirColor;
            } else if (entry.executable) {
                color = execColor;
            }

            std::cout << color << entry.name << "\033[0m";
            if (i + 1 < entries.size()) {
                std::cout << "  ";
            }
        }
        std::cout << "\n";
    };

    auto cliPrompt = [&fs]() {
        std::string path = fs.pwd();
        const std::string homePrefix = "/home/user";
        std::string displayPath;

        if (path == homePrefix) {
            displayPath = "~";
        } else if (startsWith(path, homePrefix + "/")) {
            displayPath = "~" + path.substr(homePrefix.size());
        } else {
            displayPath = path;
        }

        return "user@linuxlabx:" + displayPath + "$";
    };

    printOutput("Qt not found. Running CLI terminal fallback.", OutputType::Info);
    printOutput("Welcome beginner! Type 'help' for a friendly command guide.", OutputType::Info);
    printOutput("Then try 'tutorial' for a 5-step practice run.", OutputType::Info);
    printOutput("Commands: mkdir <name>, cd <name>, cd .., ls, rm, pwd, help, tutorial, exit", OutputType::Info);

    while (true) {
        if (awaitingDeleteConfirm) {
            std::cout << cliPrompt() << " ";
            auto future = std::async(std::launch::async, []() {
                std::string response;
                std::getline(std::cin, response);
                return response;
            });

            if (future.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
                printOutput("Delete timed out.", OutputType::Info);
                awaitingDeleteConfirm = false;
                pendingDeleteTarget.clear();
                continue;
            }

            std::string response = future.get();
            std::transform(response.begin(), response.end(), response.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });

            if (response == "y" || response == "yes") {
                Command rmCmd;
                rmCmd.name = "rm";
                rmCmd.args = {"-r", pendingDeleteTarget};
                std::string output = executor.execute(rmCmd);
                printOutput(output, OutputType::Success);
            } else {
                printOutput("Delete canceled.", OutputType::Info);
            }

            awaitingDeleteConfirm = false;
            pendingDeleteTarget.clear();
            continue;
        }

        std::cout << cliPrompt() << " ";
        if (!std::getline(std::cin, input)) {
            break;
        }

        if (startsWith(input, "suggest ")) {
            std::string partial = input.substr(8);
            std::vector<std::string> suggestions = getSuggestions(partial, fs);

            if (suggestions.empty()) {
                printOutput("(no suggestions)", OutputType::Info);
            } else {
                for (size_t i = 0; i < suggestions.size(); ++i) {
                    std::cout << suggestions[i];
                    if (i + 1 < suggestions.size()) {
                        std::cout << "  ";
                    }
                }
                std::cout << "\n";
            }
            continue;
        }

        Command cmd = parseInput(input);
        if (cmd.name.empty()) {
            continue;
        }

        if (cmd.name == "rm") {
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
                    target.clear();
                    break;
                }
                target = arg;
            }

            if (recursive && !force && !target.empty()) {
                awaitingDeleteConfirm = true;
                pendingDeleteTarget = target;
                printOutput("Confirm delete (y/n) within 5 seconds", OutputType::Info);
                continue;
            }
        }

        if (cmd.name == "exit") {
            break;
        }

        if (cmd.name == "pwd") {
            if (!cmd.args.empty()) {
                printOutput("Usage: pwd", OutputType::Error);
            } else {
                printOutput(fs.pwd(), OutputType::Normal);
            }
            continue;
        }

        if (cmd.name == "ls") {
            if (cmd.args.size() > 1) {
                printOutput("Usage: ls [path]", OutputType::Error);
                continue;
            }

            std::string path = cmd.args.empty() ? "" : cmd.args[0];
            std::string rawOutput = fs.lsPath(path);
            if (startsWith(rawOutput, "Invalid path segment")) {
                printOutput(rawOutput, OutputType::Error);
                continue;
            }
            if (rawOutput == "(empty)") {
                printOutput(rawOutput, OutputType::Normal);
                std::string suggestion = getSuggestion(cmd.name, cmd.args);
                if (!suggestion.empty()) {
                    printOutput(suggestion, OutputType::Info);
                }
                continue;
            }

            std::vector<DirEntry> entries = fs.listEntries(path);
            printLsEntries(entries);
            std::string suggestion = getSuggestion(cmd.name, cmd.args);
            if (!suggestion.empty()) {
                printOutput(suggestion, OutputType::Info);
            }
            continue;
        }

        std::string output = executor.execute(cmd);
        OutputType type = wasSuccessfulExecution(cmd, output) ? OutputType::Success : OutputType::Error;
        if (cmd.name == "ls" || cmd.name == "pwd") {
            type = OutputType::Normal;
        }
        printOutput(output, type);

        if (wasSuccessfulExecution(cmd, output)) {
            std::string suggestion = getSuggestion(cmd.name, cmd.args);
            if (!suggestion.empty()) {
                printOutput(suggestion, OutputType::Info);
            }
        }
    }

    return 0;
#endif
}
