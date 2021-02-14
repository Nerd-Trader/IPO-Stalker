/*
 * IPO Calendar data source for Japanese IPOs
 * Spec: https://ipo-cal.appspot.com/apispec.html
 *
 */

#include <QNetworkRequest>
#include <QDebug>

#include "inc/data-sources/ipo-cal-appspot.hpp"

IpoCalAppSpot::IpoCalAppSpot(QObject *parent): QObject(parent)
{
}

IpoCalAppSpot::~IpoCalAppSpot()
{
    delete reply;
}

void IpoCalAppSpot::query()
{
    QNetworkRequest request(url);

    reply = manager.get(request);

    connect(reply, SIGNAL(readyRead()), SLOT(ready()));
}

void IpoCalAppSpot::ready()
{
    QByteArray data = reply->readAll();

    qInfo() << data;

    reply->deleteLater();
}
