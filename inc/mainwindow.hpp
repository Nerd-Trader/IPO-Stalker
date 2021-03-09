#pragma once

#include <QAction>
#include <QMainWindow>
#include <QSettings>
#include <QSystemTrayIcon>

#include "data-sources.hpp"
#include "traymenu.hpp"

class DataSources; // Forward declaration

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
    static bool compareDates(const Ipo &ipo1, const Ipo &ipo2);
    QString formatDateCell(QString expectedDate);
    QString formatWebsiteCell(QString websiteUrl);
    void loadSettings();
    void setIcon();
    void setStyle();

    DataSources *dataSources;
    QSystemTrayIcon *trayIcon;
    TrayMenu *trayMenu;
    Ui::MainWindow *ui;

private slots:
    void showMessage();
    void messageClicked();
};
