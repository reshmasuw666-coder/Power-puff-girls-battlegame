// =============================================================================
//  main.cpp - application entry point
//
//  Responsibilities: create the QApplication, apply the pastel style-sheet and
//  show the main window. All game logic lives elsewhere (see GameManager).
// =============================================================================
#include "GameWindow.h"

#include <QApplication>
#include <QtGlobal>

int main(int argc, char* argv[])
{
#if defined(Q_OS_LINUX)
    // SFML embeds itself via an X11 window id. Under a Wayland session Qt would
    // hand out a Wayland surface instead, so ask Qt for its XCB (X11/XWayland)
    // backend unless the user has chosen a platform explicitly.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "xcb");
#endif

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Power Puff Girls");
    QCoreApplication::setOrganizationName("PowerPuffGirls");
    app.setStyle("Fusion");   // consistent look across platforms
    app.setStyleSheet(R"(
        QMainWindow, QDialog, QMessageBox { background: #fff1f7; }
        QLabel        { color: #7a2e57; font-size: 13px; }
        QMenuBar      { background: #ffe0ee; color: #7a2e57; }
        QMenuBar::item:selected, QMenu::item:selected { background: #ffc2dd; }
        QMenu         { background: #fff1f7; color: #7a2e57; }
        QPushButton {
            background: #ffd6e8; border: 2px solid #ff9ec8; border-radius: 12px;
            padding: 6px 16px; font-weight: bold; color: #7a2e57;
        }
        QPushButton:hover    { background: #ffe6f1; }
        QPushButton:pressed  { background: #ffc2dd; }
        QPushButton:disabled { background: #eeeeee; border-color: #dddddd; color: #aaaaaa; }
    )");

    GameWindow window;
    window.show();
    return app.exec();
}
