#pragma once

#include "data-source.hpp"
#include "data-sources/euronext.hpp"
#include "data-sources/finnhub.hpp"
#include "data-sources/ipo-cal-appspot.hpp"
#include "data-sources/nasdaq.hpp"
#include "data-sources/otcbb-swingtradebot.hpp"
#include "ipo.hpp"
#include "mainwindow.hpp"

class MainWindow; // Forward declaration

class Scraper : public QObject
{
    Q_OBJECT

public:
    explicit Scraper(QObject *parent = nullptr);
    ~Scraper();

    QVector<DataSource *> dataSources;
    MainWindow *parentObject;

public slots:
    void processRetrievedIpoData(const Ipo *ipo, const QString dataSourceName);

private:
    void start();
};
