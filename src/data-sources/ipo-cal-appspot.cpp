/*
 * IPO Calendar data source: Japanese IPOs
 * Spec: https://ipo-cal.appspot.com/apispec.html
 *
 */

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QTextCodec>

#include "data-sources/ipo-cal-appspot.hpp"

DataSourceIpoCalAppSpot::DataSourceIpoCalAppSpot(QObject *parent): QObject(parent)
{
}

DataSourceIpoCalAppSpot::~DataSourceIpoCalAppSpot()
{
    delete reply;
}

QList<Ipo> DataSourceIpoCalAppSpot::query()
{
    QNetworkRequest request(url);
    QList<Ipo> retrieved_ipos;

    // request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    reply = manager.get(request);

    while (!reply->isFinished()) {
        QCoreApplication::processEvents();
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int status = statusCode.toInt();

    if (status != 200) {
        return retrieved_ipos;
    }

    QJsonParseError jsonParseError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        return retrieved_ipos;
    }
    QJsonObject jsonRoot = jsonDocument.object();
    if (jsonRoot["result"] == QJsonValue::Undefined) {
        return retrieved_ipos;
    }
    QJsonArray dataArray = jsonRoot["data"].toArray();

    foreach (const QJsonValue &item, dataArray) {
        Ipo ipo;
        QJsonObject ipoObj = item.toObject();

        if (ipoObj["title"] == QJsonValue::Undefined) {
            continue;
        }

        ipo.company_name = ipoObj["name"].toString();
        ipo.company_website = QUrl(ipoObj["url"].toString());

        retrieved_ipos.append(ipo);
    }

    reply->deleteLater();

    return retrieved_ipos;
}
