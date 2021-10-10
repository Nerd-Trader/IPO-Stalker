#include <QAction>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QTimer>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#define COLUMN_INDEX_ID                       0
#define COLUMN_INDEX_FLAGGED                  1
#define COLUMN_INDEX_NAME                     2
#define COLUMN_INDEX_TICKER                   3
#define COLUMN_INDEX_STATUS                   4
#define COLUMN_INDEX_FILING_DATE              5
#define COLUMN_INDEX_EXPECTED_DATE            6
#define COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE 7
#define COLUMN_INDEX_REGION                   8
#define COLUMN_INDEX_EXCHANGE                 9
#define COLUMN_INDEX_SECTOR                   10
#define COLUMN_INDEX_WEBSITE                  11
#define COLUMN_INDEX_SOURCES                  12

#define COLUMN_UI_IMPORTANT_FLAG "🚩"

#define IPO_STATUS_UI_SEPARATOR     " → "
#define IPO_STATUS_FILED_UI_STR     "📝"
#define IPO_STATUS_EXPECTED_UI_STR  IPO_STATUS_FILED_UI_STR IPO_STATUS_UI_SEPARATOR "🕑"
#define IPO_STATUS_PRICED_UI_STR    IPO_STATUS_EXPECTED_UI_STR IPO_STATUS_UI_SEPARATOR "✅"
#define IPO_STATUS_WITHDRAWN_UI_STR IPO_STATUS_EXPECTED_UI_STR IPO_STATUS_UI_SEPARATOR "❌"
#define IPO_STATUS_UNKNOWN_UI_STR   ""

MainWindow::MainWindow() : QMainWindow(), ui(new Ui::MainWindow)
{
    settings = new Settings();

    {
        const QFileInfo settingsFileInfo(settings->filePath());
        const QString databaseFilePath =
            settingsFileInfo.absolutePath() + QDir::separator() + PROG_NAME ".sqlite";
        db = new Db(&databaseFilePath);
        startDate = QDateTime::currentDateTime().addMonths(-1); // TODO should update every day/hour or so;
        // connect(db, &Db::ipoRecordInsertedSignal, this, &MainWindow::processIpoRecordInsertedSlot);
        // connect(db, &Db::ipoRecordUpdatedSignal, this, &MainWindow::processIpoRecordUpdatedSlot);
        connect(db, &Db::ipoRecordsRetrievedSignal, this, &MainWindow::processIpoRecordsRetrievedSlot);
        db->start();
    }

    scraper = new Scraper(db);

    ui->setupUi(this);

    setMinimumSize(800, 400);
    setWindowIcon(QIcon(":/images/" PROG_NAME ".svg"));

    if (settings->get("geometry").size() > 0) {
        restoreGeometry(
            QByteArray::fromHex(settings->get("geometry").toLatin1())
        );
    }

    prepareTable();

    applyStyle();

    trayMenu = new TrayMenu(this);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(":/images/" PROG_NAME ".svg"));
    trayIcon->show();

    bindShortcuts();

    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            toggleHidden();
        }
    });
    connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::messageClicked);

    ready = true;

    show();

    // showMessage();
}

MainWindow::~MainWindow()
{
    delete scraper;
    delete ui;
}

void MainWindow::applyStyle()
{
    QString styleSheet;

    {
        QFile styleFile(":/stylesheets/" PROG_NAME ".qss");
        styleFile.open(QFile::ReadOnly);
        styleSheet = QLatin1String(styleFile.readAll());
        styleFile.close();
    }

    {
        QFileInfo settingsFileInfo(settings->filePath());
        QFile customStyleFile(settingsFileInfo.absolutePath() + QDir::separator() + PROG_NAME ".qss");
        if (customStyleFile.open(QFile::ReadOnly)) {
            styleSheet += QLatin1String(customStyleFile.readAll());
            customStyleFile.close();
        }
    }

    // Hacky fix to ensure that the scrollbar does not reach outside the scrolalble area into the header row
    {
        const QSize headerSize = ui->treeWidget->header()->sizeHint();
        styleSheet += QString("QScrollBar:vertical { margin-top: %1px; }").arg(headerSize.height() + 2);
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

QTreeWidgetItem* MainWindow::buildTreeWidgetItem(const Ipo* ipo)
{
    QTreeWidgetItem* ipoItem = new QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr));
    bool isWithdrawn = ipo->withdrawn_date.toString().size() > 0;

    ipoItem->setText(COLUMN_INDEX_ID,                       QString().setNum(ipo->id));
    ipoItem->setText(COLUMN_INDEX_FLAGGED,                  (ipo->is_important) ? COLUMN_UI_IMPORTANT_FLAG : NULL);
    ipoItem->setText(COLUMN_INDEX_NAME,                     ipo->company_name);
    ipoItem->setText(COLUMN_INDEX_STATUS,                   ipoStatusToString(ipo->status));
    ipoItem->setText(COLUMN_INDEX_FILING_DATE,              ipo->filed_date.toString());
    ipoItem->setText(COLUMN_INDEX_EXPECTED_DATE,            ipo->expected_date.toString());
    ipoItem->setText(COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE, (isWithdrawn) ? ipo->withdrawn_date.toString() : ipo->priced_date.toString());
    ipoItem->setText(COLUMN_INDEX_REGION,                   *prettyPrintRegion(ipo->region));
    ipoItem->setText(COLUMN_INDEX_EXCHANGE,                 ipo->stock_exchange);
    ipoItem->setText(COLUMN_INDEX_SECTOR,                   ipo->market_sector);
    ipoItem->setText(COLUMN_INDEX_TICKER,                   ipo->ticker);
    ipoItem->setText(COLUMN_INDEX_WEBSITE,                  ipo->company_website.toDisplayString());
    ipoItem->setText(COLUMN_INDEX_SOURCES,                  ipo->sources.join(", "));

    ipoItem->setTextAlignment(COLUMN_INDEX_FLAGGED, Qt::AlignCenter);

    return ipoItem;
}

bool MainWindow::checkIfThisIpoShouldBeDisplayed(const Ipo* ipo)
{
    return (ipo->filed_date.isValid() && ipo->filed_date > startDate) ||
        (ipo->expected_date.isValid() && ipo->expected_date > startDate) ||
        (ipo->priced_date.isValid() && ipo->priced_date > startDate) ||
        (ipo->withdrawn_date.isValid() && ipo->withdrawn_date < startDate);
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
    const QDateTime date = QDateTime::fromString(*timestamp);
    const QString formattedDate = date.toString(QLocale().dateFormat(QLocale::ShortFormat));

    if (date < QDateTime::currentDateTime()) {
        color = "gray";
    }

    return "<span style=\"color: " + color + "\">" + formattedDate + "</span>";
}

void MainWindow::formatTableRow(QTreeWidgetItem *treeWidgetItem)
{
    static const int date_column_indices[] = {
        COLUMN_INDEX_FILING_DATE,
        COLUMN_INDEX_EXPECTED_DATE,
        COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE,
    };

    // Highlight dates
    for (const int column_index : date_column_indices) {
        QString dateStr = treeWidgetItem->text(column_index);
        if (dateStr.size() > 0) {
            QLabel *label = new QLabel();
            treeWidgetItem->setText(column_index, NULL);
            if (column_index == COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE && treeWidgetItem->text(COLUMN_INDEX_STATUS) == ipoStatusToString(IPO_STATUS_WITHDRAWN)) {
                label->setText("<s>" + formatDateCell(&dateStr) + "</s>");
            } else {
                label->setText(formatDateCell(&dateStr));
            }
            ui->treeWidget->setItemWidget(treeWidgetItem, column_index, label);
        }
    }

    // Make website links clickable
    QString website = treeWidgetItem->text(COLUMN_INDEX_WEBSITE);
    if (website.size() > 0) {
        QLabel *label = new QLabel();
        label->setOpenExternalLinks(true);
        treeWidgetItem->setText(COLUMN_INDEX_WEBSITE, NULL);
        label->setText(formatWebsiteCell(&website));
        ui->treeWidget->setItemWidget(treeWidgetItem, COLUMN_INDEX_WEBSITE, label);
    }
}

QString MainWindow::formatWebsiteCell(const QString *websiteUrl)
{
    return "<a href=\"" + *websiteUrl + "\" style=\"color: #0de5e5\">" + *websiteUrl + "</a>";
}

QString MainWindow::ipoStatusToString(const IpoStatus status) {
    switch (status) {
        case IPO_STATUS_FILED:
            return IPO_STATUS_FILED_UI_STR;

        case IPO_STATUS_EXPECTED:
            return IPO_STATUS_EXPECTED_UI_STR;

        case IPO_STATUS_PRICED:
            return IPO_STATUS_PRICED_UI_STR;

        case IPO_STATUS_WITHDRAWN:
            return IPO_STATUS_WITHDRAWN_UI_STR;

        default:
        case IPO_STATUS_UNKNOWN:
            return IPO_STATUS_UNKNOWN_UI_STR;
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

void MainWindow::prepareTable()
{
    {
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
        header->setText(COLUMN_INDEX_SOURCES,                  "Source");
    }

    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeWidget->hideColumn(COLUMN_INDEX_ID);
    ui->treeWidget->setAlternatingRowColors(true);
    ui->treeWidget->setIndentation(false);
    ui->treeWidget->setWordWrap(false);

    QObject::connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, [=](QTreeWidgetItem *item, int column) {
        (void)(column);
        const int id = item->text(COLUMN_INDEX_ID).toInt();

        db->toggleImportant(id);

        // Update the UI
        if (item->text(COLUMN_INDEX_FLAGGED).size() > 0) {
            item->setText(COLUMN_INDEX_FLAGGED, NULL);
        } else {
            item->setText(COLUMN_INDEX_FLAGGED, COLUMN_UI_IMPORTANT_FLAG);
        }
    });
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

int MainWindow::getIndexOfExistingVisibleIpo(const Ipo* ipo)
{
    int existingIndex = -1;

    for (int i = 0; i < visibleIpos.size(); ++i) {
        const Ipo* visibleIpo = &visibleIpos.at(i);

        if (visibleIpo->id == ipo->id) {
            existingIndex = i;
            break;
        }
    }

    return existingIndex;
}

void MainWindow::processIpoRecordsRetrievedSlot(const QList<Ipo>* ipos)
{
    // Process the input
    for (int i = 0; i < ipos->size(); ++i) {
        const Ipo* ipo = &ipos->at(i);
        const int existingIndex = getIndexOfExistingVisibleIpo(ipo);

        if (checkIfThisIpoShouldBeDisplayed(ipo)) {
            if (existingIndex > -1) {
                // Replace existing record with new information
                visibleIpos.replace(existingIndex, *ipo);
            } else {
                visibleIpos.append(*ipo);
            }
        } else {
            // Check if this ipo is in visibleIpo, remove if it is
            if (existingIndex > -1) {
                visibleIpos.removeAt(existingIndex);
            }
        }
    }

    // Sort items by date
    std::sort(visibleIpos.begin(), visibleIpos.end(), sortFn);

    // Clear all previous items from the list
    ui->treeWidget->clear();

    QList<QTreeWidgetItem*> items;

    for (int i = 0; i < visibleIpos.size(); ++i) {
        const Ipo* visibleIpo = &visibleIpos.at(i);

        if (checkIfThisIpoShouldBeDisplayed(visibleIpo)) {
            items.append(buildTreeWidgetItem(visibleIpo));
        }
    }

    if (items.size() > 0) {
        ui->treeWidget->insertTopLevelItems(0, items);

        // Widgets cannot be added until the row is inserted into the tree,
        // hence styling can't be combined with the loop above :(
        foreach(QTreeWidgetItem* ipoTreeWidgetItem, items) {
            formatTableRow(ipoTreeWidgetItem);
        }
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

void MainWindow::showMessage()
{
    QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information);

    trayIcon->showMessage("Title", "Content", msgIcon, 5 * 1000);
}

bool MainWindow::sortFn(const Ipo& ipo1, const Ipo& ipo2)
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
