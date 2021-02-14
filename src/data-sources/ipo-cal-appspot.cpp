/*
 * IPO Calendar data source for Japanese IPOs
 * Spec: https://ipo-cal.appspot.com/apispec.html
 *
 */

#include <QNetworkRequest>
#include <QDebug>

#include "inc/data-sources/ipo-cal-appspot.hpp"

DataSourceIpoCalAppSpot::DataSourceIpoCalAppSpot(QObject *parent): QObject(parent)
{
}

DataSourceIpoCalAppSpot::~DataSourceIpoCalAppSpot()
{
    delete reply;
}

void DataSourceIpoCalAppSpot::query()
{
    QNetworkRequest request(url);

    reply = manager.get(request);

    connect(reply, SIGNAL(readyRead()), SLOT(ready()));
}

void DataSourceIpoCalAppSpot::ready()
{
    QByteArray data = reply->readAll();

    qInfo() << data;

    reply->deleteLater();
}
