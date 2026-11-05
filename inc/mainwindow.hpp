#pragma once

#include <QDateTime>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QTreeWidget>

#include "db.hpp"
#include "scraper.hpp"
#include "settings.hpp"
#include "traymenu.hpp"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow();

    static QString ipoStatusToString(const IpoStatus status);
    static const char* prettyPrintRegion(const IpoRegion ipoRegion);

public slots:
    void showAbout();
    void toggleHidden();
    void quitApplication();

protected:
    void closeEvent(QCloseEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    bool ready = false;

private:
    void applyStyle();
    void bindShortcuts();
    void checkNotifications();
    QTreeWidgetItem* buildTreeWidgetItem(const Ipo* ipo) const;
    bool checkIfThisIpoShouldBeDisplayed(const Ipo* ipo) const;
    const Ipo* findIpo(int ipoId) const;
    void prepareTable();
    void redrawTable();
    void showContextMenu(const QPoint& pos);
    void showIpoHistory(const Ipo* ipo);
    void updateStatusBar();

    Db* db;
    Scraper* scraper;
    Settings* settings;
    QSystemTrayIcon* trayIcon = nullptr;
    TrayMenu* trayMenu = nullptr;
    Ui::MainWindow* ui;
    QTimer* notificationTimer;
    QTimer* refreshTimer;
    QDateTime lastUpdate;
    QByteArray windowGeometry;

private slots:
    void processIpoRecordsRetrievedSlot(const QList<Ipo>& ipos);
};
