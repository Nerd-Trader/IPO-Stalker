#pragma once

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "ipo.hpp"

#define DATE_SOURCE_IPO_CAL_APPSPOT_DATE_FORMAT "yyyy/MM/dd"

class DataSourceIpoCalAppSpot : public QObject
{
    Q_OBJECT

public:
    explicit DataSourceIpoCalAppSpot(QObject *parent = nullptr);
    ~DataSourceIpoCalAppSpot();

    QList<Ipo> queryData();

private:
    QString translateSectorName(QString original);

    QDateTime lastUsed;
    QString baseUrl = QString("https://ipo-cal.appspot.com/api");
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    QUrl url = QUrl(baseUrl + "/ipo");
};
