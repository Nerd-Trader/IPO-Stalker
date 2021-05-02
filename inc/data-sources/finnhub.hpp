#pragma once

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#include "ipo.hpp"

class DataSourceFinnhub : public QObject
{
    Q_OBJECT

public:
    explicit DataSourceFinnhub(QObject *parent = nullptr, QString apiKey = "");
    ~DataSourceFinnhub();

    QList<Ipo> queryData();

private:
    QDateTime lastUsed;
    QString apiKey = "";
    QString baseUrl = QString("https://finnhub.io/api/v1");
    QNetworkAccessManager manager;
    QUrlQuery query;
    QNetworkReply *reply;
    QUrl url = QUrl(baseUrl + "/calendar/ipo");
};
