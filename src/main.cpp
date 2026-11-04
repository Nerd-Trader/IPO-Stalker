#include <QApplication>
#include <QDebug>

#include "mainwindow.hpp"
#include "runguard.hpp"

#define UNIQUE_STRING "9SzFd77+9Nljvv73TqO"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

QString getUserName()
{
    QString name = qEnvironmentVariable("USER");

    if (name.isEmpty()) {
        name = qEnvironmentVariable("USERNAME");
    }

    return name;
}

int main(int argc, char **argv)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Account for running on high-DPI displays (always enabled in Qt 6)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication application(argc, argv);
    QApplication::setApplicationName(PROG_NAME);
    QApplication::setApplicationDisplayName("IPO Stalker");
    QApplication::setApplicationVersion(STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH));
    QApplication::setDesktopFileName(PROG_NAME);
    // The program keeps running in the system tray when its window gets closed
    QApplication::setQuitOnLastWindowClosed(false);

    RunGuard guard(PROG_NAME "-" UNIQUE_STRING "-" + getUserName());

    if (!guard.tryToRun()) {
        qWarning().noquote() << PROG_NAME " is already running";
        return 1;
    }

    MainWindow window;

    return application.exec();
}
