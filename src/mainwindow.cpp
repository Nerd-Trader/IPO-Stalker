#include <QCloseEvent>
#include <QDebug>
#include <QFileInfo>
#include <QLabel>
#include <QSettings>
#include <QShortcut>
#include <QTimer>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    loadSettings();

    setMinimumSize(800, 400);

    setIcon();

    setStyle();

    bindShortcuts();

    trayMenu = new TrayMenu(this);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(":/images/" TARGET ".svg"));
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason)
    {
        if (reason == QSystemTrayIcon::Trigger) {
            toggleHidden();
        }
    });
    connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::messageClicked);

    ready = true;

    show();

    // showMessage();

    QTreeWidgetItem *header = ui->treeWidget->headerItem();
    header->setText(COLUMN_INDEX_FLAG,           "");
    header->setText(COLUMN_INDEX_NAME,           "Company Name");
    header->setText(COLUMN_INDEX_STATUS,         "Status");
    header->setText(COLUMN_INDEX_FILED_DATE,     "Filed");
    header->setText(COLUMN_INDEX_EXPECTED_DATE,  "Expected");
    header->setText(COLUMN_INDEX_PRICED_DATE,    "Listed");
    header->setText(COLUMN_INDEX_WITHDRAWN_DATE, "Withdrawn");
    header->setText(COLUMN_INDEX_REGION,         "Region");
    header->setText(COLUMN_INDEX_EXCHANGE,       "Exchange");
    header->setText(COLUMN_INDEX_SECTOR,         "Market Sector");
    header->setText(COLUMN_INDEX_TICKER,         "Ticker");
    header->setText(COLUMN_INDEX_WEBSITE,        "Company Website");
    header->setText(COLUMN_INDEX_SOURCES,        "Sources");

    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeWidget->setAlternatingRowColors(true);
    ui->treeWidget->setIndentation(false);
    ui->treeWidget->setWordWrap(false);

    dataSources = new DataSources(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::sortIPOs(const Ipo &ipo1, const Ipo &ipo2)
{
    // 1. Sort by status
    if (ipo1.status != ipo2.status) {
        return ipo1.status < ipo2.status;
    }

    // 2. Sort by dates if status is the same
    QDateTime l = ipo1.filed_date;
    QDateTime r = ipo2.filed_date;

    if (ipo1.expected_date > l) {
        l = ipo1.expected_date;
    }
    if (ipo1.priced_date > l) {
        l = ipo1.priced_date;
    }
    if (ipo1.withdrawn_date > l) {
        l = ipo1.withdrawn_date;
    }

    if (ipo2.expected_date > r) {
        r = ipo2.expected_date;
    }
    if (ipo2.priced_date > r) {
        r = ipo2.priced_date;
    }
    if (ipo2.withdrawn_date > r) {
        r = ipo2.withdrawn_date;
    }

    return l > r;
}

QString MainWindow::formatDateCell(QString timestamp)
{
    QString color = "white";
    QDateTime date = QDateTime::fromString(timestamp);
    QString formattedDate = date.toString(QLocale().dateFormat(QLocale::ShortFormat));

    if (date < QDateTime::currentDateTime()) {
        color = "gray";
    }

    return "<span style=\"color: " + color + "\">" + formattedDate + "</span>";
}

QString MainWindow::formatWebsiteCell(QString websiteUrl)
{
    return "<a href=\"" + websiteUrl + "\" style=\"color: #0de5e5\">" + websiteUrl + "</a>";
}

QString MainWindow::ipoStatusToString(IpoStatus status) {
    switch (status) {
        case IPO_STATUS_FILED:
            return "📝";

        case IPO_STATUS_EXPECTED:
            return "📝 → 🕑";

        case IPO_STATUS_PRICED:
            return "📝 → 🕑 → ✅";

        case IPO_STATUS_WITHDRAWN:
            return "📝 → 🕑 → ❌";

        default:
        case IPO_STATUS_UNKNOWN:
            return "";
    }
}

void MainWindow::updateList()
{
    // Clear all previous items from the list
    while (ui->treeWidget->topLevelItemCount() > 0) {
        delete ui->treeWidget->takeTopLevelItem(0);
    }

    qSort(ipos.begin(), ipos.end(), sortIPOs);

    QList<QTreeWidgetItem *> items;

    foreach(Ipo ipo, ipos) {
        QTreeWidgetItem *ipoItem = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr));
        ipoItem->setText(COLUMN_INDEX_NAME,           ipo.company_name);
        ipoItem->setText(COLUMN_INDEX_STATUS,         ipoStatusToString(ipo.status));
        ipoItem->setText(COLUMN_INDEX_FILED_DATE,     ipo.filed_date.toString());
        ipoItem->setText(COLUMN_INDEX_EXPECTED_DATE,  ipo.expected_date.toString());
        ipoItem->setText(COLUMN_INDEX_PRICED_DATE,    ipo.priced_date.toString());
        ipoItem->setText(COLUMN_INDEX_WITHDRAWN_DATE, ipo.withdrawn_date.toString());
        ipoItem->setText(COLUMN_INDEX_REGION,         ipo.region);
        ipoItem->setText(COLUMN_INDEX_EXCHANGE,       ipo.stock_exchange);
        ipoItem->setText(COLUMN_INDEX_SECTOR,         ipo.market_sector);
        ipoItem->setText(COLUMN_INDEX_TICKER,         ipo.ticker);
        ipoItem->setText(COLUMN_INDEX_WEBSITE,        ipo.company_website.toDisplayString());
        ipoItem->setText(COLUMN_INDEX_SOURCES,        ipo.sources.join(", "));
        items.append(ipoItem);
    }

    ui->treeWidget->insertTopLevelItems(0, items);

    // Highlight dates
    foreach(QTreeWidgetItem *ipoItem, items) {
        int date_column_indices[] = { COLUMN_INDEX_FILED_DATE, COLUMN_INDEX_EXPECTED_DATE, COLUMN_INDEX_PRICED_DATE, COLUMN_INDEX_WITHDRAWN_DATE };

        for (auto column_index : date_column_indices) {
            QString date = ipoItem->text(column_index);

            if (date.size() > 0) {
                QLabel *label = new QLabel();
                label->setOpenExternalLinks(true);
                ipoItem->setText(column_index, NULL);
                label->setText(formatDateCell(date));
                ui->treeWidget->setItemWidget(ipoItem, column_index, label);
            }
        }
    }

    // Make website links clickable
    foreach(QTreeWidgetItem *ipoItem, items) {
        QString website = ipoItem->text(COLUMN_INDEX_WEBSITE);

        if (website.size() > 0) {
            QLabel *label = new QLabel();
            label->setOpenExternalLinks(true);
            ipoItem->setText(COLUMN_INDEX_WEBSITE, NULL);
            label->setText(formatWebsiteCell(website));
            ui->treeWidget->setItemWidget(ipoItem, COLUMN_INDEX_WEBSITE, label);
        }
    }
}

void MainWindow::loadSettings()
{
    settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                             TARGET, TARGET, nullptr);

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

    setStyleSheet(styleSheet);
}

void MainWindow::bindShortcuts()
{
    // Connect the quit shortcut (Ctrl+Q)
    QAction *quitAction = new QAction(this);
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    addAction(quitAction);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quitApplication()));
}

void MainWindow::showMessage()
{
    QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information);

    trayIcon->showMessage("Title", "Content", msgIcon, 5 * 1000);
}

void MainWindow::messageClicked()
{
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
