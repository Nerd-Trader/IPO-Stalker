#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QShortcut>
#include <QTimer>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#define COLUMN_INDEX_ID 0
#define COLUMN_INDEX_FLAGGED 1
#define COLUMN_INDEX_NAME 2
#define COLUMN_INDEX_TICKER 3
#define COLUMN_INDEX_STATUS 4
#define COLUMN_INDEX_FILING_DATE 5
#define COLUMN_INDEX_EXPECTED_DATE 6
#define COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE 7
#define COLUMN_INDEX_REGION 8
#define COLUMN_INDEX_EXCHANGE 9
#define COLUMN_INDEX_SECTOR 10
#define COLUMN_INDEX_WEBSITE 11
#define COLUMN_INDEX_SOURCES 12

MainWindow::MainWindow() : QMainWindow(), ui(new Ui::MainWindow)
{
    settings = new Settings();

    {
        QFileInfo settingsFileInfo(settings->filePath());
        db = new Db(settingsFileInfo.absolutePath() + QDir::separator() + PROG_NAME + ".sqlite");
    }

    scraper = new Scraper(this);

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

    // Populate table based on data obtained from the database
    updateList();

    // showMessage();

    QTreeWidgetItem *header = ui->treeWidget->headerItem();
    header->setText(COLUMN_INDEX_FLAGGED,                  "");
    header->setText(COLUMN_INDEX_NAME,                     "Company Name");
    header->setText(COLUMN_INDEX_STATUS,                   "Status");
    header->setText(COLUMN_INDEX_FILING_DATE,              "Filed");
    header->setText(COLUMN_INDEX_EXPECTED_DATE,            "Expected");
    header->setText(COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE, "Listed");
    header->setText(COLUMN_INDEX_REGION,                   "Region");
    header->setText(COLUMN_INDEX_EXCHANGE,                 "Exchange");
    header->setText(COLUMN_INDEX_SECTOR,                   "Market Sector");
    header->setText(COLUMN_INDEX_TICKER,                   "Ticker");
    header->setText(COLUMN_INDEX_WEBSITE,                  "Company Website");
    header->setText(COLUMN_INDEX_SOURCES,                  "Source(s)");

    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeWidget->hideColumn(COLUMN_INDEX_ID);
    ui->treeWidget->setAlternatingRowColors(true);
    ui->treeWidget->setIndentation(false);
    ui->treeWidget->setWordWrap(false);

    QObject::connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, [=](QTreeWidgetItem *item, int column) {
        (void)(column);
        const int id = item->text(COLUMN_INDEX_ID).toInt();
        db->toggleImportant(id);
        updateList();
    });
}

MainWindow::~MainWindow()
{
    delete scraper;
    delete ui;
}

void MainWindow::bindShortcuts()
{
    // Connect the quit shortcut (Ctrl+Q)
    QAction *quitAction = new QAction(this);
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    addAction(quitAction);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quitApplication()));
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

QString MainWindow::formatDateCell(const QString *timestamp)
{
    QString color = "white";
    QDateTime date = QDateTime::fromString(*timestamp);
    QString formattedDate = date.toString(QLocale().dateFormat(QLocale::ShortFormat));

    if (date < QDateTime::currentDateTime()) {
        color = "gray";
    }

    return "<span style=\"color: " + color + "\">" + formattedDate + "</span>";
}

QString MainWindow::formatWebsiteCell(const QString *websiteUrl)
{
    return "<a href=\"" + *websiteUrl + "\" style=\"color: #0de5e5\">" + *websiteUrl + "</a>";
}

QString MainWindow::ipoStatusToString(const IpoStatus status) {
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

void MainWindow::messageClicked()
{
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    if (ready) {
        windowGeometry = saveGeometry();
    }

    QMainWindow::moveEvent(event);
}

QString *MainWindow::prettyPrintRegion(const IpoRegion ipoRegion)
{
    static QString region_asia_japan = "🇯🇵 Asia (Japan)";
    static QString region_europe_belgium = "🇧🇪 Europe (Belgium)";
    static QString region_europe_france = "🇫🇷 Europe (France)";
    static QString region_europe_ireland = "🇮🇪 Europe (Ireland)";
    static QString region_europe_italy = "🇮🇹 Europe (Italy)";
    static QString region_europe_netherlands = "🇳🇱 Europe (Netherlands)";
    static QString region_europe_norway = "🇳🇴 Europe (Norway)";
    static QString region_europe_portugal = "🇵🇹 Europe (Portugal)";
    static QString region_europe_uk = "🇬🇧 Europe (UK)";
    static QString region_na_usa = "🇺🇸 North America (US)";
    static QString region_global = "🌎 Global";
    static QString region_unknown = "Unknown";

    switch (ipoRegion) {
        case IPO_REGION_COUNTRY_JAPAN:
            return &region_asia_japan;

        case IPO_REGION_COUNTRY_BELGIUM:
            return &region_europe_belgium;

        case IPO_REGION_COUNTRY_FRANCE:
            return &region_europe_france;

        case IPO_REGION_COUNTRY_IRELAND:
            return &region_europe_ireland;

        case IPO_REGION_COUNTRY_ITALY:
            return &region_europe_italy;

        case IPO_REGION_COUNTRY_NETHERLANDS:
            return &region_europe_netherlands;

        case IPO_REGION_COUNTRY_NORWAY:
            return &region_europe_norway;

        case IPO_REGION_COUNTRY_PORTUGAL:
            return &region_europe_portugal;

        case IPO_REGION_COUNTRY_UK:
            return &region_europe_uk;

        case IPO_REGION_COUNTRY_USA:
            return &region_na_usa;

        case IPO_REGION_GLOBAL:
            return &region_global;

        default:
        case IPO_REGION_UNKNOWN:
            return &region_unknown;
    }
}

void MainWindow::quitApplication()
{
    settings->set("geometry", QString(windowGeometry.toHex()));

    QApplication::quit();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (ready) {
        windowGeometry = saveGeometry();
    }

    QMainWindow::resizeEvent(event);
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
    QFile customStyleFile(settingsFileInfo.absolutePath() + QDir::separator() + PROG_NAME + ".qss");
    if (customStyleFile.open(QFile::ReadOnly)) {
        styleSheet += QLatin1String(customStyleFile.readAll());
        customStyleFile.close();
    }

    setStyleSheet(styleSheet);
}

void MainWindow::showMessage()
{
    QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information);

    trayIcon->showMessage("Title", "Content", msgIcon, 5 * 1000);
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

void MainWindow::updateList()
{
    // Clear all previous items from the list
    ui->treeWidget->clear();

    QList<QTreeWidgetItem *> items;
    foreach(const Ipo ipo, db->ipos) {
        QTreeWidgetItem *ipoItem = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr));
        bool isWithdrawn = ipo.withdrawn_date.toString().size() > 0;

        ipoItem->setText(COLUMN_INDEX_ID,                       QString().setNum(ipo.id));
        ipoItem->setText(COLUMN_INDEX_FLAGGED,                  (ipo.is_important) ? "🚩" : NULL);
        ipoItem->setText(COLUMN_INDEX_NAME,                     ipo.company_name);
        ipoItem->setText(COLUMN_INDEX_STATUS,                   ipoStatusToString(ipo.status));
        ipoItem->setText(COLUMN_INDEX_FILING_DATE,              ipo.filed_date.toString());
        ipoItem->setText(COLUMN_INDEX_EXPECTED_DATE,            ipo.expected_date.toString());
        ipoItem->setText(COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE, (isWithdrawn) ? ipo.withdrawn_date.toString() : ipo.priced_date.toString());
        ipoItem->setText(COLUMN_INDEX_REGION,                   *prettyPrintRegion(ipo.region));
        ipoItem->setText(COLUMN_INDEX_EXCHANGE,                 ipo.stock_exchange);
        ipoItem->setText(COLUMN_INDEX_SECTOR,                   ipo.market_sector);
        ipoItem->setText(COLUMN_INDEX_TICKER,                   ipo.ticker);
        ipoItem->setText(COLUMN_INDEX_WEBSITE,                  ipo.company_website.toDisplayString());
        ipoItem->setText(COLUMN_INDEX_SOURCES,                  ipo.sources.join(", "));

        ipoItem->setTextAlignment(COLUMN_INDEX_FLAGGED, Qt::AlignCenter);

        items.append(ipoItem);
    }
    ui->treeWidget->insertTopLevelItems(0, items);

    // Widgets cannot be added until the row is added to the tree, so this cannot be combined with the loop above :(
    static const int date_column_indices[] = { COLUMN_INDEX_FILING_DATE, COLUMN_INDEX_EXPECTED_DATE, COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE };
    foreach(QTreeWidgetItem *ipoItem, items) {
        // Highlight dates
        for (int column_index : date_column_indices) {
            QString dateStr = ipoItem->text(column_index);
            if (dateStr.size() > 0) {
                QLabel *label = new QLabel();
                ipoItem->setText(column_index, NULL);
                if (column_index == COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE && ipoItem->text(COLUMN_INDEX_STATUS) == ipoStatusToString(IPO_STATUS_WITHDRAWN)) {
                    label->setText("<s>" + formatDateCell(&dateStr) + "</s>");
                } else {
                    label->setText(formatDateCell(&dateStr));
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
            label->setText(formatWebsiteCell(&website));
            ui->treeWidget->setItemWidget(ipoItem, COLUMN_INDEX_WEBSITE, label);
        }
    }
}
