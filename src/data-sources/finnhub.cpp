/*
 * IPO Calendar data source: US IPOs
 * API Spec: https://finnhub.io/docs/api
 *
 */

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QTextCodec>

#include "data-sources/finnhub.hpp"

DataSourceFinnhub::DataSourceFinnhub(QObject *parent, QString apiKey) : QObject(parent)
{
    query.addQueryItem("from", "2021-01-01");
    query.addQueryItem("to", "2021-06-01");
    url.setQuery(query.query());
    this->apiKey = apiKey;
}

DataSourceFinnhub::~DataSourceFinnhub()
{
    delete reply;
}

QList<Ipo> DataSourceFinnhub::queryData()
{
    QNetworkRequest request(url);
    QList<Ipo> retrievedIpos;

    lastUsed = QDateTime::currentDateTime();

    if (apiKey.isEmpty()) {
        return retrievedIpos;
    }

    request.setRawHeader("X-Finnhub-Token", apiKey.toUtf8());

    reply = manager.get(request);

    while (!reply->isFinished()) {
        QCoreApplication::processEvents();
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int status = statusCode.toInt();

    if (status == 200) {
        QJsonParseError jsonParseError;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError) {
            return retrievedIpos;
        }
        QJsonObject jsonRoot = jsonDocument.object();
        if (jsonRoot["ipoCalendar"] == QJsonValue::Undefined) {
            return retrievedIpos;
        }
        QJsonArray dataArray = jsonRoot["ipoCalendar"].toArray();

        foreach (const QJsonValue &item, dataArray) {
            Ipo ipo;
            QJsonObject ipoObj = item.toObject();

            if (ipoObj["name"] == QJsonValue::Undefined) {
                continue;
            }

            ipo.company_name = ipoObj["name"].toString();
            // ipo.company_website = QUrl("https://ddg.gg/?q=" + ipo.company_name);
            ipo.expected_date = QDateTime::fromString(ipoObj["date"].toString(), "yyyy-MM-dd");
            ipo.region = QString("🇺🇸 North America (US)");
            ipo.status = ipoObj["status"].toString();
            ipo.stock_exchange = ipoObj["exchange"].toString();
            ipo.ticker = ipoObj["symbol"].toString();

            retrievedIpos.append(ipo);
        }
    }

    reply->deleteLater();

    return retrievedIpos;
}
