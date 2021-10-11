#pragma once

#include <QThread>

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

class Scraper : public QThread
{
    Q_OBJECT

public:
    explicit Scraper(Db* db = nullptr);
    ~Scraper();

    QVector<DataSource*> dataSources;

public slots:
    void startSlot();

private:
    Db *db;
};
