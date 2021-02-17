#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "ipo.hpp"

class DataSourceIpoCalAppSpot : public QObject
{
    Q_OBJECT

public:
    explicit DataSourceIpoCalAppSpot(QObject *parent = 0);
    ~DataSourceIpoCalAppSpot();

    QList<Ipo> query();

private:
    QString baseUrl = QString("https://ipo-cal.appspot.com/api");
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    QUrl url = QUrl(baseUrl + "/ipo");
};
