#include "logging.h"
#include "ui.h"
#include "version.h"

#include <iostream>
#include <stdexcept>
#include <string>

#if __has_include(<QMessageBox>)
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#define HAS_QT_DIALOGS 1
#else
#define HAS_QT_DIALOGS 0
#endif

namespace {
int showCrashDialog(const std::string& reason, const std::string& logPath) {
#if HAS_QT_DIALOGS
    QMessageBox crashBox;
    crashBox.setIcon(QMessageBox::Critical);
    crashBox.setWindowTitle("LinuxLabX");
    crashBox.setText("Something went wrong");
    crashBox.setInformativeText(QString::fromStdString(reason + "\n\nLog file: " + logPath));

    QPushButton* viewLogs = crashBox.addButton("View logs", QMessageBox::ActionRole);
    crashBox.addButton("Close", QMessageBox::AcceptRole);
    crashBox.exec();

    if (crashBox.clickedButton() == viewLogs && !logPath.empty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(logPath)));
    }
    return 1;
#else
    std::cerr << "Something went wrong: " << reason << "\n";
    std::cerr << "Log file: " << logPath << "\n";
    return 1;
#endif
}
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        std::cout << "LinuxLabX " << LINUXLABX_VERSION_STRING << " (" << LINUXLABX_BUILD_TIMESTAMP << ")" << std::endl;
        return 0;
    }

    logging::init("LinuxLabX", LINUXLABX_VERSION_STRING);
    logging::info("Application start requested");

    try {
        return runTerminalApp(argc, argv);
    } catch (const std::exception& ex) {
        logging::error(std::string("Unhandled exception: ") + ex.what());
        return showCrashDialog(ex.what(), logging::getLogFilePath());
    } catch (...) {
        logging::error("Unhandled unknown exception");
        return showCrashDialog("Unhandled unknown exception", logging::getLogFilePath());
    }
}
