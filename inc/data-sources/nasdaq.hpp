#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "ipo.hpp"

class DataSourceNasdaq: public QObject
{
    Q_OBJECT

public:
    explicit DataSourceNasdaq(QObject *parent = nullptr);
    ~DataSourceNasdaq();

    QList<Ipo> queryData();

private:
    QString getCurrentDate();

    QString baseUrl = QString("https://api.nasdaq.com/api");
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    QUrl url = QUrl(baseUrl + "/ipo/calendar");
};
