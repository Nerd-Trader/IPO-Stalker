#pragma once

#include <QAction>
#include <QMainWindow>
#include <QSystemTrayIcon>

#include "db.hpp"
#include "scraper.hpp"
#include "settings.hpp"
#include "traymenu.hpp"

class Scraper; // Forward declaration

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow();

    Db *db;

public slots:
    void toggleHidden();
    void quitApplication();
    void updateList();

protected:
    void closeEvent(QCloseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    bool ready = false;

private:
    void bindShortcuts();
    static QString *prettyPrintRegion(const IpoRegion ipoRegion);
    static QString formatDateCell(const QString *expectedDate);
    static QString formatWebsiteCell(const QString *websiteUrl);
    static QString ipoStatusToString(const IpoStatus status);
    void setIcon();
    void setStyle();

    Scraper *scraper;
    Settings *settings;
    QSystemTrayIcon *trayIcon;
    TrayMenu *trayMenu;
    Ui::MainWindow *ui;
    QByteArray windowGeometry;

private slots:
    void messageClicked();
    void showMessage();
};
