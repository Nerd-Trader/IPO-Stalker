#pragma once

#include <QList>
#include <QNetworkAccessManager>
#include <QObject>

#include "data-source.hpp"
#include "db.hpp"
#include "ipo.hpp"

class Scraper : public QObject
{
    Q_OBJECT

public:
    explicit Scraper(Db* db, QObject* parent = nullptr);
    ~Scraper();

    void start();

    QList<DataSource*> dataSources;

private:
    Db* db;
    QNetworkAccessManager* networkAccessManager;
};
