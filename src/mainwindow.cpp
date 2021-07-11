#include <QCloseEvent>
#include <QDebug>
#include <QFileInfo>
#include <QLabel>
#include <QShortcut>
#include <QTimer>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    settings = new Settings();
    ui->setupUi(this);

    setMinimumSize(800, 400);

    if (settings->get("geometry").size() > 0) {
        restoreGeometry(
            QByteArray::fromHex(settings->get("geometry").toLatin1())
        );
    }

    setIcon();

    setStyle();

    bindShortcuts();

    trayMenu = new TrayMenu(this);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(":/images/" PROG_NAME ".svg"));
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
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
    header->setText(COLUMN_INDEX_PRICED_OR_WITHDRAWN_DATE, "Listed");
    header->setText(COLUMN_INDEX_REGION,         "Region");
    header->setText(COLUMN_INDEX_EXCHANGE,       "Exchange");
    header->setText(COLUMN_INDEX_SECTOR,         "Market Sector");
    header->setText(COLUMN_INDEX_TICKER,         "Ticker");
    header->setText(COLUMN_INDEX_WEBSITE,        "Company Website");
    header->setText(COLUMN_INDEX_SOURCES,        "Source(s)");

    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeWidget->setAlternatingRowColors(true);
    ui->treeWidget->setIndentation(false);
    ui->treeWidget->setWordWrap(false);

    scraper = new Scraper(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::sortIPOs(const Ipo &ipo1, const Ipo &ipo2)
{
    // Push filed/unknown below everything else
    // if (ipo1.status != ipo2.status && (ipo1.status > IPO_STATUS_WITHDRAWN || ipo2.status > IPO_STATUS_WITHDRAWN)) {
    //     return !(ipo1.status > IPO_STATUS_WITHDRAWN);
    // }

    // Sort by dates
    QDateTime l = ipo1.filed_date;
    QDateTime r = ipo2.filed_date;
    // Determine which date to use on the left side
    if (ipo1.expected_date > l) {
        l = ipo1.expected_date;
    }
    if (ipo1.priced_date > l) {
        l = ipo1.priced_date;
    } else if (ipo1.withdrawn_date > l) {
        l = ipo1.withdrawn_date;
    }
    // Determine which date to use on the right side
    if (ipo2.expected_date > r) {
        r = ipo2.expected_date;
    }
    if (ipo2.priced_date > r) {
        r = ipo2.priced_date;
    } else if (ipo2.withdrawn_date > r) {
        r = ipo2.withdrawn_date;
    }
    // Compare dates
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
        bool isWithdrawn = ipo.withdrawn_date.toString().size() > 0;

        ipoItem->setText(COLUMN_INDEX_NAME,           ipo.company_name);
        ipoItem->setText(COLUMN_INDEX_STATUS,         ipoStatusToString(ipo.status));
        ipoItem->setText(COLUMN_INDEX_FILED_DATE,     ipo.filed_date.toString());
        ipoItem->setText(COLUMN_INDEX_EXPECTED_DATE,  ipo.expected_date.toString());
        ipoItem->setText(COLUMN_INDEX_PRICED_OR_WITHDRAWN_DATE, (isWithdrawn) ? ipo.withdrawn_date.toString() : ipo.priced_date.toString());
        ipoItem->setText(COLUMN_INDEX_REGION,         ipo.region);
        ipoItem->setText(COLUMN_INDEX_EXCHANGE,       ipo.stock_exchange);
        ipoItem->setText(COLUMN_INDEX_SECTOR,         ipo.market_sector);
        ipoItem->setText(COLUMN_INDEX_TICKER,         ipo.ticker);
        ipoItem->setText(COLUMN_INDEX_WEBSITE,        ipo.company_website.toDisplayString());
        ipoItem->setText(COLUMN_INDEX_SOURCES,        ipo.sources.join(", "));

        items.append(ipoItem);
    }
    ui->treeWidget->insertTopLevelItems(0, items);

    // Widgets cannot be added until the row has been added to the tree, so this cannot be combined with the loop above :(
    int date_column_indices[] = { COLUMN_INDEX_FILED_DATE, COLUMN_INDEX_EXPECTED_DATE, COLUMN_INDEX_PRICED_OR_WITHDRAWN_DATE };
    foreach(QTreeWidgetItem *ipoItem, items) {
        // Highlight dates
        for (int column_index : date_column_indices) {
            QString dateStr = ipoItem->text(column_index);
            if (dateStr.size() > 0) {
                QLabel *label = new QLabel();
                ipoItem->setText(column_index, NULL);
                if (column_index == COLUMN_INDEX_PRICED_OR_WITHDRAWN_DATE && ipoItem->text(COLUMN_INDEX_STATUS) == ipoStatusToString(IPO_STATUS_WITHDRAWN)) {
                    label->setText("<s>" + formatDateCell(dateStr) + "</s>");
                } else {
                    label->setText(formatDateCell(dateStr));
                }
                ui->treeWidget->setItemWidget(ipoItem, column_index, label);
            }
        }

        // Make website links clickable
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

void MainWindow::setIcon()
{
    QIcon windowIcon(":/images/" PROG_NAME ".svg");

    setWindowIcon(windowIcon);
}

void MainWindow::setStyle()
{
    QString styleSheet;

    QFile styleFile(":/styles/" PROG_NAME ".qss");
    styleFile.open(QFile::ReadOnly);
    styleSheet = QLatin1String(styleFile.readAll());
    styleFile.close();

    QFileInfo settingsFileInfo(settings->filePath());
    QFile customStyleFile(settingsFileInfo.absolutePath() + "/" PROG_NAME ".qss");
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
        settings->set("geometry", QString(saveGeometry().toHex()));
    }

    QMainWindow::resizeEvent(event);
}
