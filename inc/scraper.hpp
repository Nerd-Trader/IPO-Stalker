#pragma once

#include <QThread>

#include "data-source.hpp"
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

    QList<DataSource*> dataSources;

public slots:
    void startSlot();

private:
    Db *db;
};
