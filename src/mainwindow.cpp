#include <QSettings>
#include <QCloseEvent>
#include <QShortcut>
#include <QTimer>
#include <QFileInfo>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    loadSettings();

    setMinimumSize(240, 360);

    setIcon();

    setStyle();
    ui->textArea->setFrameStyle(QFrame::NoFrame);

    bindShortcuts();

    trayMenu = new TrayMenu(this);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(":/images/tray.png"));
    trayIcon->show();

    QObject::connect(trayIcon, &QSystemTrayIcon::activated, this,
                     [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            toggleHidden();
        }
    });

    ready = true;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadSettings()
{
    settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                             TARGET, TARGET, nullptr);

    if (settings->contains("text")) {
        QString content = settings->value("text").toString();

        ui->textArea->setPlainText(content);
    }

    if (settings->contains("geometry")) {
        restoreGeometry(
            QByteArray::fromHex(settings->value("geometry").toByteArray())
        );
    }
}

void MainWindow::setIcon()
{
    QIcon windowIcon(":/images/" TARGET ".svg");

    setWindowIcon(windowIcon);
}

void MainWindow::setStyle()
{
    QString styleSheet;

    QFile styleFile(":/styles/" TARGET ".qss");
    styleFile.open(QFile::ReadOnly);
    styleSheet = QLatin1String(styleFile.readAll());
    styleFile.close();

    QFileInfo settingsFileInfo(settings->fileName());
    QFile customStyleFile(settingsFileInfo.absolutePath() + "/" TARGET ".qss");
    if (customStyleFile.open(QFile::ReadOnly)) {
        styleSheet += QLatin1String(customStyleFile.readAll());
        customStyleFile.close();
    }

    ui->textArea->setStyleSheet(styleSheet);
}

void MainWindow::bindShortcuts()
{
    // Connect the quit shortcut (Ctrl+Q)
    QAction *quitAction = new QAction(this);
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    addAction(quitAction);
    connect(quitAction, SIGNAL(triggered()),
            this, SLOT(quitApplication()));
}

void MainWindow::toggleHidden()
{
    if (isVisible()) {
        if (isActiveWindow()) {
            QTimer::singleShot(0, this, [this]() {
                hide();
            });
        } else {
            if (isMinimized()) {
                setWindowState(
                    (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive
                );
                raise();
            }
            activateWindow();
        }
    } else {
        QTimer::singleShot(0, this, [this]() {
            ensurePolished();
            setWindowState(windowState() & ~Qt::WindowMinimized);
            show();
            raise();
            activateWindow();
        });
    }
}

void MainWindow::quitApplication()
{
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (isVisible()) {
        event->ignore();

        QTimer::singleShot(0, this, SLOT(hide()));
    } else {
        quitApplication();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (ready) {
        settings->setValue("geometry", QString(saveGeometry().toHex()));
    }

    QMainWindow::resizeEvent(event);
}

void MainWindow::on_textArea_textChanged()
{
    if (ready) {
        settings->setValue("text", ui->textArea->toPlainText());
    }
}
