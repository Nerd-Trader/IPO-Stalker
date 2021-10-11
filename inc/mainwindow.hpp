#pragma once

#include <QAction>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTreeWidget>

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

public slots:
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
    static QTreeWidgetItem* buildTreeWidgetItem(const Ipo* ipo);
    bool checkIfThisIpoShouldBeDisplayed(const Ipo* ipo);
    static QString formatDateCell(const QString* expectedDate);
    void formatTableRow(QTreeWidgetItem* treeWidgetItem);
    static QString formatWebsiteCell(const QString* websiteUrl);
    int getIndexOfExistingVisibleIpo(const Ipo* ipo);
    static QString ipoStatusToString(const IpoStatus status);
    void prepareTable();
    static const char* prettyPrintRegion(const IpoRegion ipoRegion);
    static bool sortFn(const Ipo& ipo1, const Ipo& ipo2);

    Db* db;
    Scraper* scraper;
    Settings* settings;
    QDateTime startDate;
    QSystemTrayIcon* trayIcon;
    TrayMenu* trayMenu;
    Ui::MainWindow* ui;
    QList<Ipo> visibleIpos;
    QByteArray windowGeometry;

private slots:
    void messageClicked();
    void showMessage();
    void processIpoRecordsRetrievedSlot(const QList<Ipo>* ipos);
};
