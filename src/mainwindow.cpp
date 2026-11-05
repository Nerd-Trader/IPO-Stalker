#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLocale>
#include <QMessageBox>
#include <QMenu>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "common.hpp"
#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#define UI_TABLE_COLUMN_INDEX_ID                       0
#define UI_TABLE_COLUMN_INDEX_IMPORTANT                1
#define UI_TABLE_COLUMN_INDEX_NAME                     2
#define UI_TABLE_COLUMN_INDEX_TICKER                   3
#define UI_TABLE_COLUMN_INDEX_STATUS                   4
#define UI_TABLE_COLUMN_INDEX_FILING_DATE              5
#define UI_TABLE_COLUMN_INDEX_EXPECTED_DATE            6
#define UI_TABLE_COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE 7
#define UI_TABLE_COLUMN_INDEX_REGION                   8
#define UI_TABLE_COLUMN_INDEX_EXCHANGE                 9
#define UI_TABLE_COLUMN_INDEX_SECTOR                   10
#define UI_TABLE_COLUMN_INDEX_WEBSITE                  11

#define UI_IPO_IMPORTANT_STR "🚩"

#define UI_ROLE_SORT_KEY  (Qt::UserRole + 1)
#define UI_ROLE_IPO_ID    (Qt::UserRole + 2)

#define UI_VISIBLE_HISTORY_DAYS   31
#define UI_REFRESH_INTERVAL       (15 * 60) // Seconds

#define NOTIFICATION_CHECK_INTERVAL 60 // Seconds
#define NOTIFICATION_DURATION       15 // Seconds

#define UI_IPO_STATUS_SEPARATOR_STR " → "
#define UI_IPO_STATUS_FILED_STR     "📝"
#define UI_IPO_STATUS_EXPECTED_STR  UI_IPO_STATUS_FILED_STR UI_IPO_STATUS_SEPARATOR_STR "🕑"
#define UI_IPO_STATUS_PRICED_STR    UI_IPO_STATUS_EXPECTED_STR UI_IPO_STATUS_SEPARATOR_STR "✅"
#define UI_IPO_STATUS_WITHDRAWN_STR UI_IPO_STATUS_EXPECTED_STR UI_IPO_STATUS_SEPARATOR_STR "❌"
#define UI_IPO_STATUS_UNKNOWN_STR   ""

#define UI_IPO_REGION_EA_JAPAN_STR       "🇯🇵 East Asia (Japan)"
#define UI_IPO_REGION_EU_BELGIUM_STR     "🇧🇪 Europe (Belgium)"
#define UI_IPO_REGION_EU_FRANCE_STR      "🇫🇷 Europe (France)"
#define UI_IPO_REGION_EU_IRELAND_STR     "🇮🇪 Europe (Ireland)"
#define UI_IPO_REGION_EU_ITALY_STR       "🇮🇹 Europe (Italy)"
#define UI_IPO_REGION_EU_NETHERLANDS_STR "🇳🇱 Europe (Netherlands)"
#define UI_IPO_REGION_EU_NORWAY_STR      "🇳🇴 Europe (Norway)"
#define UI_IPO_REGION_EU_PORTUGAL_STR    "🇵🇹 Europe (Portugal)"
#define UI_IPO_REGION_EU_UK_STR          "🇬🇧 Europe (UK)"
#define UI_IPO_REGION_NA_CANADA_STR      "🇨🇦 North America (Canada)"
#define UI_IPO_REGION_NA_USA_STR         "🇺🇸 North America (US)"
#define UI_IPO_REGION_OC_AUSTRALIA_STR   "🇦🇺 Oceania (Australia)"
#define UI_IPO_REGION_SA_INDIA_STR       "🇮🇳 South Asia (India)"
#define UI_IPO_REGION_GLOBAL_STR         "🌎 Global"
#define UI_IPO_REGION_UNKNOWN_STR        "Unknown"

const struct IntKeyStrValPair ipoRegionKeyUiIpoRegionValTable[] = {
    { IPO_REGION_EA_JAPAN,       UI_IPO_REGION_EA_JAPAN_STR },
    { IPO_REGION_EU_BELGIUM,     UI_IPO_REGION_EU_BELGIUM_STR },
    { IPO_REGION_EU_FRANCE,      UI_IPO_REGION_EU_FRANCE_STR },
    { IPO_REGION_EU_IRELAND,     UI_IPO_REGION_EU_IRELAND_STR },
    { IPO_REGION_EU_ITALY,       UI_IPO_REGION_EU_ITALY_STR },
    { IPO_REGION_EU_NETHERLANDS, UI_IPO_REGION_EU_NETHERLANDS_STR },
    { IPO_REGION_EU_NORWAY,      UI_IPO_REGION_EU_NORWAY_STR },
    { IPO_REGION_EU_PORTUGAL,    UI_IPO_REGION_EU_PORTUGAL_STR },
    { IPO_REGION_EU_UK,          UI_IPO_REGION_EU_UK_STR },
    { IPO_REGION_NA_CANADA,      UI_IPO_REGION_NA_CANADA_STR },
    { IPO_REGION_NA_USA,         UI_IPO_REGION_NA_USA_STR },
    { IPO_REGION_OC_AUSTRALIA,   UI_IPO_REGION_OC_AUSTRALIA_STR },
    { IPO_REGION_SA_INDIA,       UI_IPO_REGION_SA_INDIA_STR },
    { IPO_REGION_GLOBAL,         UI_IPO_REGION_GLOBAL_STR },
    { IPO_REGION_UNKNOWN,        UI_IPO_REGION_UNKNOWN_STR },
};

namespace {

// Tree widget item which sorts by hidden sort keys (dates, flags) rather than by display text
class IpoTreeWidgetItem : public QTreeWidgetItem
{
public:
    IpoTreeWidgetItem() : QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr)) {}

    bool operator<(const QTreeWidgetItem& other) const override
    {
        const int column = treeWidget() ? treeWidget()->sortColumn() : 0;
        const QVariant l = data(column, UI_ROLE_SORT_KEY);
        const QVariant r = other.data(column, UI_ROLE_SORT_KEY);

        if (l.isValid() || r.isValid()) {
            return l.toString() < r.toString();
        }

        return text(column).compare(other.text(column), Qt::CaseInsensitive) < 0;
    }
};

QString dateSortKey(const QDateTime& date)
{
    // Empty dates sort before everything else
    return date.isValid() ? date.toString("yyyyMMddHHmm") : QString("0");
}

// The most relevant date of an IPO, used for default ordering
QDateTime mostRecentDate(const Ipo* ipo)
{
    QDateTime date = ipo->filed_date;

    if (ipo->expected_date > date) {
        date = ipo->expected_date;
    }
    if (ipo->priced_date > date) {
        date = ipo->priced_date;
    }
    if (ipo->withdrawn_date > date) {
        date = ipo->withdrawn_date;
    }

    return date;
}

}

MainWindow::MainWindow() : QMainWindow(), ui(new Ui::MainWindow)
{
    settings = new Settings();

    {
        const QFileInfo settingsFileInfo(settings->filePath());
        const QString dbDirPath = settingsFileInfo.absolutePath();
        db = new Db(dbDirPath, this);
        connect(db, &Db::ipoRecordsRetrievedSignal, this, &MainWindow::processIpoRecordsRetrievedSlot);
    }

    scraper = new Scraper(db, this);

    ui->setupUi(this);

    setMinimumSize(800, 400);
    setWindowIcon(QIcon(":/images/" PROG_NAME ".svg"));

    if (settings->get("geometry").size() > 0) {
        restoreGeometry(
            QByteArray::fromHex(settings->get("geometry").toLatin1())
        );
    }
    windowGeometry = saveGeometry();

    prepareTable();

    applyStyle();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayMenu = new TrayMenu(this);
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setContextMenu(trayMenu);
        trayIcon->setIcon(QIcon(":/images/" PROG_NAME ".svg"));
        trayIcon->setToolTip(windowTitle());
        trayIcon->show();

        connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
                toggleHidden();
            }
        });
        connect(trayIcon, &QSystemTrayIcon::messageClicked, this, [this]() {
            if (!isVisible() || !isActiveWindow()) {
                toggleHidden();
            }
        });
    }

    bindShortcuts();

    // Version in the bottom right corner
    {
        QToolButton* versionButton = new QToolButton(this);
        versionButton->setObjectName("versionButton");
        versionButton->setText("v" PROG_VERSION);
        versionButton->setToolTip(tr("About IPO Stalker (F1)"));
        versionButton->setAutoRaise(true);
        versionButton->setCursor(Qt::PointingHandCursor);
        connect(versionButton, &QToolButton::clicked, this, &MainWindow::showAbout);
        statusBar()->addPermanentWidget(versionButton);
    }

    // Periodically re-evaluate which IPOs are visible and which dates are in the past
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::redrawTable);
    refreshTimer->start(UI_REFRESH_INTERVAL * 1000);

    // Remind about flagged IPOs that are about to happen
    notificationTimer = new QTimer(this);
    connect(notificationTimer, &QTimer::timeout, this, &MainWindow::checkNotifications);
    notificationTimer->start(NOTIFICATION_CHECK_INTERVAL * 1000);

    ready = true;

    db->readRecords();
    redrawTable();
    checkNotifications();

    show();

    scraper->start();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete settings;
}

void MainWindow::applyStyle()
{
    QString styleSheet;

    {
        QFile styleFile(":/stylesheets/" PROG_NAME ".qss");
        if (styleFile.open(QFile::ReadOnly)) {
            styleSheet = QString::fromUtf8(styleFile.readAll());
            styleFile.close();
        }
    }

    {
        QFileInfo settingsFileInfo(settings->filePath());
        QFile customStyleFile(settingsFileInfo.absolutePath() + QDir::separator() + PROG_NAME ".qss");
        if (customStyleFile.open(QFile::ReadOnly)) {
            styleSheet += QString::fromUtf8(customStyleFile.readAll());
            customStyleFile.close();
        }
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Hacky fix to ensure that the scrollbar does not reach outside the scrollable area into the header row
    {
        const QSize headerSize = ui->treeWidget->header()->sizeHint();
        styleSheet += QString("QScrollBar:vertical { margin-top: %1px; }").arg(headerSize.height() + 2);
    }
#endif

    setStyleSheet(styleSheet);
}

void MainWindow::bindShortcuts()
{
    // Connect the quit shortcut (Ctrl+Q)
    QAction* quitAction = new QAction(this);
    quitAction->setShortcut(QKeySequence::Quit);
    if (quitAction->shortcut().isEmpty()) {
        quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    }
    addAction(quitAction);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);

    // About dialog (F1)
    QAction* aboutAction = new QAction(this);
    aboutAction->setShortcut(QKeySequence::HelpContents);
    if (aboutAction->shortcut().isEmpty()) {
        aboutAction->setShortcut(QKeySequence(Qt::Key_F1));
    }
    addAction(aboutAction);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    // Hide the window (Escape)
    QAction* hideAction = new QAction(this);
    hideAction->setShortcut(QKeySequence(Qt::Key_Escape));
    addAction(hideAction);
    connect(hideAction, &QAction::triggered, this, [this]() {
        if (trayIcon != nullptr) {
            hide();
        }
    });
}

void MainWindow::checkNotifications()
{
    // Hours before the expected date at which to remind about flagged IPOs
    static const int thresholds[] = { 48, 24, 1 };

    if (trayIcon == nullptr || !QSystemTrayIcon::supportsMessages()) {
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    QStringList messages;

    for (const Ipo& ipo : db->ipos()) {
        if (!ipo.is_important || !ipo.expected_date.isValid() ||
            ipo.status == IPO_STATUS_PRICED || ipo.status == IPO_STATUS_WITHDRAWN) {
            continue;
        }

        const qint64 secondsLeft = now.secsTo(ipo.expected_date);
        if (secondsLeft < 0) {
            continue;
        }

        // Find the smallest threshold already reached
        int reached = 0;
        for (const int threshold : thresholds) {
            if (secondsLeft <= threshold * 3600) {
                reached = threshold;
            }
        }

        // last_notification holds the last threshold notified about (0 = none yet)
        if (reached == 0 || (ipo.last_notification > 0 && ipo.last_notification <= reached)) {
            continue;
        }

        const int hoursLeft = static_cast<int>((secondsLeft + 1800) / 3600);
        QString name = ipo.company_name;
        if (!ipo.ticker.isEmpty()) {
            name += QString(" (%1)").arg(ipo.ticker);
        }
        QString message = hoursLeft > 0 ?
            tr("%1 is expected to IPO in about %n hour(s)", "", hoursLeft).arg(name) :
            tr("%1 is expected to IPO now").arg(name);
        if (!ipo.stock_exchange.isEmpty()) {
            message += " " + tr("on %1").arg(ipo.stock_exchange);
        }
        messages << message;

        db->setLastNotification(ipo.id, reached);
    }

    if (!messages.isEmpty()) {
        trayIcon->showMessage(
            tr("Upcoming IPO", "", messages.size()),
            messages.join("\n"),
            QSystemTrayIcon::Information,
            NOTIFICATION_DURATION * 1000
        );
    }
}

QTreeWidgetItem* MainWindow::buildTreeWidgetItem(const Ipo* ipo) const
{
    QTreeWidgetItem* ipoItem = new IpoTreeWidgetItem();
    const QString dateFormat = QLocale().dateFormat(QLocale::ShortFormat);
    const QDateTime now = QDateTime::currentDateTime();
    const QColor pastDateColor = palette().color(QPalette::Disabled, QPalette::Text);
    const bool isWithdrawn = ipo->status == IPO_STATUS_WITHDRAWN || ipo->withdrawn_date.isValid();

    ipoItem->setText(UI_TABLE_COLUMN_INDEX_ID, QString::number(ipo->id));
    ipoItem->setData(UI_TABLE_COLUMN_INDEX_ID, UI_ROLE_IPO_ID, ipo->id);
    ipoItem->setText(UI_TABLE_COLUMN_INDEX_IMPORTANT, ipo->is_important ? UI_IPO_IMPORTANT_STR : "");
    ipoItem->setData(UI_TABLE_COLUMN_INDEX_IMPORTANT, UI_ROLE_SORT_KEY, ipo->is_important ? "1" : "0");
    ipoItem->setText(UI_TABLE_COLUMN_INDEX_NAME, ipo->company_name);
    ipoItem->setText(UI_TABLE_COLUMN_INDEX_TICKER, ipo->ticker);
    ipoItem->setText(UI_TABLE_COLUMN_INDEX_STATUS, ipoStatusToString(ipo->status));
    ipoItem->setData(UI_TABLE_COLUMN_INDEX_STATUS, UI_ROLE_SORT_KEY, QString::number(ipo->status));
    ipoItem->setText(UI_TABLE_COLUMN_INDEX_REGION, prettyPrintRegion(ipo->region));
    ipoItem->setText(UI_TABLE_COLUMN_INDEX_EXCHANGE, ipo->stock_exchange);
    ipoItem->setText(UI_TABLE_COLUMN_INDEX_SECTOR, ipo->market_sector);
    ipoItem->setText(UI_TABLE_COLUMN_INDEX_WEBSITE, ipo->company_website.toDisplayString());
    if (!ipo->company_website.isEmpty()) {
        QFont font = ipoItem->font(UI_TABLE_COLUMN_INDEX_WEBSITE);
        font.setUnderline(true);
        ipoItem->setFont(UI_TABLE_COLUMN_INDEX_WEBSITE, font);
        ipoItem->setForeground(UI_TABLE_COLUMN_INDEX_WEBSITE, QColor("#0de5e5"));
        ipoItem->setToolTip(UI_TABLE_COLUMN_INDEX_WEBSITE, tr("Double-click to open"));
    }

    // Dates
    {
        const struct { int column; QDateTime date; } dateCells[] = {
            { UI_TABLE_COLUMN_INDEX_FILING_DATE,              ipo->filed_date },
            { UI_TABLE_COLUMN_INDEX_EXPECTED_DATE,            ipo->expected_date },
            { UI_TABLE_COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE, isWithdrawn ? ipo->withdrawn_date : ipo->priced_date },
        };

        for (const auto& cell : dateCells) {
            ipoItem->setData(cell.column, UI_ROLE_SORT_KEY, dateSortKey(cell.date));

            if (!cell.date.isValid()) {
                continue;
            }

            ipoItem->setText(cell.column, QLocale().toString(cell.date.date(), dateFormat));
            if (cell.date < now) {
                ipoItem->setForeground(cell.column, pastDateColor);
            }
        }

        if (isWithdrawn) {
            QFont font = ipoItem->font(UI_TABLE_COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE);
            font.setStrikeOut(true);
            ipoItem->setFont(UI_TABLE_COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE, font);
        }
    }

    ipoItem->setTextAlignment(UI_TABLE_COLUMN_INDEX_IMPORTANT, Qt::AlignCenter);
    ipoItem->setToolTip(UI_TABLE_COLUMN_INDEX_NAME, tr("Sources: %1").arg(ipo->sources.join(", ")));

    return ipoItem;
}

bool MainWindow::checkIfThisIpoShouldBeDisplayed(const Ipo* ipo) const
{
    const QDateTime startDate = QDateTime::currentDateTime().addDays(-UI_VISIBLE_HISTORY_DAYS);

    // Always keep flagged IPOs that haven't happened yet
    if (ipo->is_important && ipo->status != IPO_STATUS_PRICED && ipo->status != IPO_STATUS_WITHDRAWN) {
        return true;
    }

    return (ipo->filed_date.isValid() && ipo->filed_date > startDate) ||
        (ipo->expected_date.isValid() && ipo->expected_date > startDate) ||
        (ipo->priced_date.isValid() && ipo->priced_date > startDate) ||
        (ipo->withdrawn_date.isValid() && ipo->withdrawn_date > startDate);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (trayIcon != nullptr && isVisible()) {
        // Minimize to tray
        event->ignore();

        QTimer::singleShot(0, this, &QWidget::hide);
    } else {
        // No tray to hide into, closing the window means quitting
        event->ignore();

        quitApplication();
    }
}

const Ipo* MainWindow::findIpo(int ipoId) const
{
    for (const Ipo& ipo : db->ipos()) {
        if (ipo.id == ipoId) {
            return &ipo;
        }
    }

    return nullptr;
}

QString MainWindow::ipoStatusToString(const IpoStatus status) {
    switch (status) {
        case IPO_STATUS_FILED:
            return UI_IPO_STATUS_FILED_STR;

        case IPO_STATUS_EXPECTED:
            return UI_IPO_STATUS_EXPECTED_STR;

        case IPO_STATUS_PRICED:
            return UI_IPO_STATUS_PRICED_STR;

        case IPO_STATUS_WITHDRAWN:
            return UI_IPO_STATUS_WITHDRAWN_STR;

        default:
        case IPO_STATUS_UNKNOWN:
            return UI_IPO_STATUS_UNKNOWN_STR;
    }
}

void MainWindow::moveEvent(QMoveEvent* event)
{
    if (ready && isVisible()) {
        windowGeometry = saveGeometry();
    }

    QMainWindow::moveEvent(event);
}

void MainWindow::prepareTable()
{
    {
        QTreeWidgetItem* header = ui->treeWidget->headerItem();
        header->setText(UI_TABLE_COLUMN_INDEX_IMPORTANT,                "");
        header->setText(UI_TABLE_COLUMN_INDEX_NAME,                     tr("Company Name"));
        header->setText(UI_TABLE_COLUMN_INDEX_STATUS,                   tr("Status"));
        header->setText(UI_TABLE_COLUMN_INDEX_FILING_DATE,              tr("Filed"));
        header->setText(UI_TABLE_COLUMN_INDEX_EXPECTED_DATE,            tr("Expected"));
        header->setText(UI_TABLE_COLUMN_INDEX_LISTED_OR_WITHDRAWN_DATE, tr("Listed"));
        header->setText(UI_TABLE_COLUMN_INDEX_REGION,                   tr("Region"));
        header->setText(UI_TABLE_COLUMN_INDEX_EXCHANGE,                 tr("Exchange"));
        header->setText(UI_TABLE_COLUMN_INDEX_SECTOR,                   tr("Market Sector"));
        header->setText(UI_TABLE_COLUMN_INDEX_TICKER,                   tr("Ticker"));
        header->setText(UI_TABLE_COLUMN_INDEX_WEBSITE,                  tr("Company Website"));
    }

    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSortIndicatorShown(true);
    ui->treeWidget->hideColumn(UI_TABLE_COLUMN_INDEX_ID);
    ui->treeWidget->setAlternatingRowColors(true);
    ui->treeWidget->setIndentation(0);
    ui->treeWidget->setRootIsDecorated(false);
    ui->treeWidget->setUniformRowHeights(true);
    ui->treeWidget->setWordWrap(false);
    ui->treeWidget->setSortingEnabled(true);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // Restore sorting order, newest first by default
    {
        bool ok = false;
        int sortColumn = settings->get("sortColumn").toInt(&ok);
        if (!ok || sortColumn <= UI_TABLE_COLUMN_INDEX_ID || sortColumn > UI_TABLE_COLUMN_INDEX_WEBSITE) {
            sortColumn = -1;
        }
        const Qt::SortOrder sortOrder = settings->get("sortOrder") == "asc" ? Qt::AscendingOrder : Qt::DescendingOrder;
        // -1 means "by most relevant date", stored in the hidden ID column's sort key
        ui->treeWidget->sortByColumn(sortColumn < 0 ? UI_TABLE_COLUMN_INDEX_ID : sortColumn, sortOrder);
    }

    connect(ui->treeWidget->header(), &QHeaderView::sortIndicatorChanged, this, [this](int column, Qt::SortOrder order) {
        settings->set("sortColumn", QString::number(column));
        settings->set("sortOrder", order == Qt::AscendingOrder ? "asc" : "desc");
    });

    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int column) {
        const int id = item->data(UI_TABLE_COLUMN_INDEX_ID, UI_ROLE_IPO_ID).toInt();
        const Ipo* ipo = findIpo(id);

        if (ipo == nullptr) {
            return;
        }

        if (column == UI_TABLE_COLUMN_INDEX_WEBSITE && !ipo->company_website.isEmpty()) {
            QDesktopServices::openUrl(ipo->company_website);
            return;
        }

        db->toggleImportant(id);

        // Update the UI
        item->setText(UI_TABLE_COLUMN_INDEX_IMPORTANT, ipo->is_important ? UI_IPO_IMPORTANT_STR : "");
        item->setData(UI_TABLE_COLUMN_INDEX_IMPORTANT, UI_ROLE_SORT_KEY, ipo->is_important ? "1" : "0");
    });

    connect(ui->treeWidget, &QWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
}

const char* MainWindow::prettyPrintRegion(const IpoRegion ipoRegion)
{
    for (const IntKeyStrValPair &ipoRegionKeyUiIpoRegionValPair : ipoRegionKeyUiIpoRegionValTable) {
        if (ipoRegionKeyUiIpoRegionValPair.key == ipoRegion) {
            return ipoRegionKeyUiIpoRegionValPair.str;
        }
    }

    return UI_IPO_REGION_UNKNOWN_STR;
}

void MainWindow::processIpoRecordsRetrievedSlot(const QList<Ipo>& ipos)
{
    (void)ipos;

    lastUpdate = QDateTime::currentDateTime();

    // The database keeps the authoritative in-memory copy of all records
    if (ready) {
        redrawTable();
        checkNotifications();
    }
}

void MainWindow::quitApplication()
{
    if (isVisible()) {
        windowGeometry = saveGeometry();
    }
    settings->set("geometry", QString(windowGeometry.toHex()));

    QApplication::quit();
}

void MainWindow::redrawTable()
{
    // Preserve selection and scroll position across redraws
    int selectedId = -1;
    if (QTreeWidgetItem* current = ui->treeWidget->currentItem()) {
        selectedId = current->data(UI_TABLE_COLUMN_INDEX_ID, UI_ROLE_IPO_ID).toInt();
    }
    const int scrollPosition = ui->treeWidget->verticalScrollBar()->value();

    QList<QTreeWidgetItem*> items;

    for (const Ipo& ipo : db->ipos()) {
        if (checkIfThisIpoShouldBeDisplayed(&ipo)) {
            QTreeWidgetItem* item = buildTreeWidgetItem(&ipo);
            // Default ordering (hidden column) is by most relevant date
            item->setData(UI_TABLE_COLUMN_INDEX_ID, UI_ROLE_SORT_KEY, dateSortKey(mostRecentDate(&ipo)));
            items.append(item);
        }
    }

    ui->treeWidget->setUpdatesEnabled(false);
    ui->treeWidget->clear();
    ui->treeWidget->insertTopLevelItems(0, items);

    if (selectedId > -1) {
        for (QTreeWidgetItem* item : items) {
            if (item->data(UI_TABLE_COLUMN_INDEX_ID, UI_ROLE_IPO_ID).toInt() == selectedId) {
                ui->treeWidget->setCurrentItem(item);
                break;
            }
        }
    }
    ui->treeWidget->verticalScrollBar()->setValue(scrollPosition);
    ui->treeWidget->setUpdatesEnabled(true);

    updateStatusBar();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    if (ready && isVisible()) {
        windowGeometry = saveGeometry();
    }

    QMainWindow::resizeEvent(event);
}

void MainWindow::showAbout()
{
    if (!isVisible()) {
        toggleHidden();
    }

    QMessageBox::about(this, tr("About IPO Stalker"), QString(
        "<h3>IPO Stalker %1</h3>"
        "<p>%2</p>"
        "<p>%3</p>"
        "<p><a href=\"https://github.com/Nerd-Trader/IPO-Stalker\">github.com/Nerd-Trader/IPO-Stalker</a></p>"
        "<p><small>%4</small></p>"
    ).arg(
        PROG_VERSION,
        tr("Reminder system for upcoming IPOs."),
        tr("Built with Qt %1, running on Qt %2.").arg(QT_VERSION_STR, qVersion()),
        tr("Licensed under the GNU General Public License v3.0.")
    ));
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = ui->treeWidget->itemAt(pos);

    if (item == nullptr) {
        return;
    }

    const int id = item->data(UI_TABLE_COLUMN_INDEX_ID, UI_ROLE_IPO_ID).toInt();
    const Ipo* ipo = findIpo(id);

    if (ipo == nullptr) {
        return;
    }

    QMenu menu(this);

    menu.addAction(ipo->is_important ? tr("Unflag") : tr("Flag (notify before IPO)"), this, [this, id]() {
        db->toggleImportant(id);
        redrawTable();
    });

    if (!ipo->company_website.isEmpty()) {
        const QUrl website = ipo->company_website;
        menu.addAction(tr("Open company website"), this, [website]() {
            QDesktopServices::openUrl(website);
        });
    }

    menu.addSeparator();

    if (!ipo->ticker.isEmpty()) {
        const QString ticker = ipo->ticker;
        menu.addAction(tr("Copy ticker"), this, [ticker]() {
            QApplication::clipboard()->setText(ticker);
        });
    }
    {
        const QString name = ipo->company_name;
        menu.addAction(tr("Copy company name"), this, [name]() {
            QApplication::clipboard()->setText(name);
        });
    }

    menu.addSeparator();

    menu.addAction(tr("Show history…"), this, [this, id]() {
        if (const Ipo* ipo = findIpo(id)) {
            showIpoHistory(ipo);
        }
    });

    menu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
}

void MainWindow::showIpoHistory(const Ipo* ipo)
{
    QDialog* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(ipo->company_name);
    dialog->resize(700, 300);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QPlainTextEdit* text = new QPlainTextEdit(dialog);
    text->setReadOnly(true);
    text->setLineWrapMode(QPlainTextEdit::NoWrap);
    text->setPlainText(
        tr("Sources: %1").arg(ipo->sources.join(", ")) + "\n\n" + ipo->log
    );
    layout->addWidget(text);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    layout->addWidget(buttons);

    dialog->show();
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
            }

            raise();
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

void MainWindow::updateStatusBar()
{
    const int count = ui->treeWidget->topLevelItemCount();
    QString message = (count == 1) ? tr("1 IPO shown") : tr("%1 IPOs shown").arg(count);

    if (lastUpdate.isValid()) {
        message += " · " + tr("last update at %1").arg(QLocale().toString(lastUpdate.time(), QLocale::ShortFormat));
    }

    statusBar()->showMessage(message);
}
