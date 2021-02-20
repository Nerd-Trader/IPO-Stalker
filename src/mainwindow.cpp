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
    // ui->textArea->setFrameStyle(QFrame::NoFrame);

    bindShortcuts();

    trayMenu = new TrayMenu(this);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(":/images/" TARGET ".svg"));
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason)
    {
        if (reason == QSystemTrayIcon::Trigger) {
            toggleHidden();
        }
    });
    connect(trayIcon, &QSystemTrayIcon::messageClicked, this,
            &MainWindow::messageClicked);

    ready = true;

    show();

    // showMessage();

    QTreeWidgetItem *header = ui->treeWidget->headerItem();
    header->setText(0, "Company Name");
    header->setText(1, "Expected Date");
    header->setText(2, "Region");
    header->setText(3, "Exchange");
    header->setText(4, "Ticker");
    header->setText(5, "Company Website");
    header->setText(6, "Status");

    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeWidget->setIndentation(0);

    dataSources = new DataSources(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::compareDates(const Ipo &ipo1, const Ipo &ipo2)
{
    return ipo1.expected_date > ipo2.expected_date;
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

void MainWindow::updateList()
{
    QList<QTreeWidgetItem *> items;

    // Clear all previous items from the list
    while (ui->treeWidget->topLevelItemCount() > 0) {
        delete ui->treeWidget->takeTopLevelItem(0);
    }

    qSort(ipos.begin(), ipos.end(), compareDates);

    foreach(Ipo ipo, ipos) {
        QTreeWidgetItem *ipoItem = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr));
        ipoItem->setText(0, ipo.company_name);
        ipoItem->setText(1, ipo.expected_date.toString());
        ipoItem->setText(2, ipo.region);
        ipoItem->setText(3, ipo.stock_exchange);
        ipoItem->setText(4, ipo.ticker);
        ipoItem->setText(5, ipo.company_website.toDisplayString());
        ipoItem->setText(6, ipo.status);
        items.append(ipoItem);
    }

    ui->treeWidget->insertTopLevelItems(0, items);

    // Highlight dates
    foreach(QTreeWidgetItem *ipoItem, items) {
        QString date = ipoItem->text(1);

        if (date.size() > 0) {
            QLabel *label = new QLabel();
            label->setOpenExternalLinks(true);
            ipoItem->setText(1, NULL);
            label->setText(formatDateCell(date));
            ui->treeWidget->setItemWidget(ipoItem, 1, label);
        }
    }

    // Make website links clickable
    foreach(QTreeWidgetItem *ipoItem, items) {
        QString website = ipoItem->text(5);

        if (website.size() > 0) {
            QLabel *label = new QLabel();
            label->setOpenExternalLinks(true);
            ipoItem->setText(5, NULL);
            label->setText(formatWebsiteCell(website));
            ui->treeWidget->setItemWidget(ipoItem, 5, label);
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
