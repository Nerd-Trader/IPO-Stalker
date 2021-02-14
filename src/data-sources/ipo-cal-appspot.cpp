/*
 * IPO Calendar data source for Japanese IPOs
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
#include "ipo.hpp"

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
    // request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    reply = manager.get(request);

    while (!reply->isFinished()) {
        QCoreApplication::processEvents();
    }

    // QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    // int status = statusCode.toInt();
    // qDebug() << status << statusCode;

    QJsonParseError jsonParseError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        return;
    }
    QJsonObject jsonRoot = jsonDocument.object();
    if (jsonRoot["result"] == QJsonValue::Undefined) {
        return;
    }
    QJsonArray dataArray = jsonRoot["data"].toArray();

    foreach (const QJsonValue &item, dataArray) {
        Ipo ipo;
        QJsonObject ipoObj = item.toObject();

        if (ipoObj["title"] == QJsonValue::Undefined) {
            return;
        }

        ipo.company_name = ipoObj["name"].toString();
        ipo.company_website = QUrl(ipoObj["url"].toString());

        qInfo() << ipo.company_name << '|' << ipo.company_website;

        // TODO: upsert newly found IPO into the global store of IPOs
    }

    reply->deleteLater();
}
