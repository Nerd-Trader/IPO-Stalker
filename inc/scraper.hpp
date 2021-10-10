#pragma once

#include "data-source.hpp"
#include "data-sources/euronext.hpp"
#include "data-sources/finnhub.hpp"
#include "data-sources/ipo-cal-appspot.hpp"
#include "data-sources/nasdaq.hpp"
#include "data-sources/otcbb-swingtradebot.hpp"
#include "db.hpp"
#include "ipo.hpp"
#include "mainwindow.hpp"

class MainWindow; // Forward declaration

class Scraper : public QObject
{
    Q_OBJECT

public:
    explicit Scraper(Db* db = nullptr);
    ~Scraper();

    QVector<DataSource*> dataSources;

public slots:
    void processRetrievedIpoData(const QList<Ipo>* ipo, const QString dataSourceName);

private:
    void start();

    Db *db;
};
