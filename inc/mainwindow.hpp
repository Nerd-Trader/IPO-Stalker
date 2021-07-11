#pragma once

#include <QAction>
#include <QMainWindow>
#include <QSettings>
#include <QSystemTrayIcon>

#include "scraper.hpp"
#include "traymenu.hpp"

#define COLUMN_INDEX_FLAG 0
#define COLUMN_INDEX_NAME 1
#define COLUMN_INDEX_TICKER 2
#define COLUMN_INDEX_STATUS 3
#define COLUMN_INDEX_FILED_DATE 4
#define COLUMN_INDEX_EXPECTED_DATE 5
#define COLUMN_INDEX_PRICED_DATE 6
#define COLUMN_INDEX_WITHDRAWN_DATE 7
#define COLUMN_INDEX_REGION 8
#define COLUMN_INDEX_EXCHANGE 9
#define COLUMN_INDEX_SECTOR 10
#define COLUMN_INDEX_WEBSITE 11
#define COLUMN_INDEX_SOURCES 12

class Scraper; // Forward declaration

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QList<Ipo> ipos;
    QSettings *settings;

public slots:
    void toggleHidden();
    void quitApplication();
    void updateList();

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event) override;

    bool ready = false;

private:
    void bindShortcuts();
    static bool sortIPOs(const Ipo &ipo1, const Ipo &ipo2);
    QString formatDateCell(QString expectedDate);
    QString formatWebsiteCell(QString websiteUrl);
    QString ipoStatusToString(IpoStatus status);
    void loadSettings();
    void setIcon();
    void setStyle();

    Scraper *scraper;
    QSystemTrayIcon *trayIcon;
    TrayMenu *trayMenu;
    Ui::MainWindow *ui;

private slots:
    void showMessage();
    void messageClicked();
};
