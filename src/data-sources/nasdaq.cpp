/*
 * IPO Calendar data source: US IPOs
 *
 */

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QTextCodec>
#include <QUrlQuery>

#include "data-sources/nasdaq.hpp"

DataSourceNasdaq::DataSourceNasdaq(QObject *parent) : QObject(parent)
{
}

DataSourceNasdaq::~DataSourceNasdaq()
{
    delete reply;
}

QList<Ipo> DataSourceNasdaq::queryData()
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("date", "2021-03"); // XXX
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    QList<Ipo> retrievedIpos;

    reply = manager.get(request);

    while (!reply->isFinished()) {
        QCoreApplication::processEvents();
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int status = statusCode.toInt();

    if (status != 200) {
        return retrievedIpos;
    }

    QJsonParseError jsonParseError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &jsonParseError);

    if (jsonParseError.error != QJsonParseError::NoError) {
        return retrievedIpos;
    }

    QJsonObject jsonRoot = jsonDocument.object();

    if (jsonRoot["data"] == QJsonValue::Undefined) {
        return retrievedIpos;
    }

    QJsonObject dataObj = jsonRoot["data"].toObject();

    if (dataObj["priced"] != QJsonValue::Undefined) {
        QJsonObject pricedObj = dataObj["priced"].toObject();

        if (pricedObj["rows"] != QJsonValue::Undefined) {
            QJsonArray pricedRowsArray = pricedObj["rows"].toArray();

            foreach (const QJsonValue &item, pricedRowsArray) {
                Ipo ipo;
                QJsonObject ipoObj = item.toObject();

                ipo.company_name = ipoObj["companyName"].toString();
                ipo.expected_date = QDateTime::fromString(ipoObj["pricedDate"].toString(), "MM/dd/yyyy");
                ipo.region = QString("🇺🇸 North America (US)");
                ipo.stock_exchange = ipoObj["proposedExchange"].toString();
                ipo.ticker = ipoObj["proposedTickerSymbol"].toString();
                ipo.status = "priced";

                retrievedIpos.append(ipo);
            }
        }
    }

    if (dataObj["upcoming"] != QJsonValue::Undefined) {
        QJsonObject upcomingObj = dataObj["upcoming"].toObject();

        if (upcomingObj["upcomingTable"] != QJsonValue::Undefined) {
            QJsonObject upcomingTableObj = dataObj["upcomingTable"].toObject();

            if (upcomingTableObj["rows"] != QJsonValue::Undefined) {
                QJsonArray upcomingRowsArray = upcomingObj["rows"].toArray();

                foreach (const QJsonValue &item, upcomingRowsArray) {
                    Ipo ipo;
                    QJsonObject ipoObj = item.toObject();

                    ipo.company_name = ipoObj["companyName"].toString();
                    ipo.expected_date = QDateTime::fromString(ipoObj["expectedPricedDate"].toString(), "MM/dd/yyyy");
                    ipo.region = QString("🇺🇸 North America (US)");
                    ipo.stock_exchange = ipoObj["proposedExchange"].toString();
                    ipo.ticker = ipoObj["proposedTickerSymbol"].toString();
                    ipo.status = "upcoming";

                    retrievedIpos.append(ipo);
                }
            }
        }
    }

    if (dataObj["filed"] != QJsonValue::Undefined) {
        QJsonObject filedObj = dataObj["filed"].toObject();

        if (filedObj["rows"] != QJsonValue::Undefined) {
            QJsonArray filedRowsArray = filedObj["rows"].toArray();

            foreach (const QJsonValue &item, filedRowsArray) {
                Ipo ipo;
                QJsonObject ipoObj = item.toObject();

                ipo.company_name = ipoObj["companyName"].toString();
                ipo.expected_date = QDateTime::fromString(ipoObj["filedDate"].toString(), "MM/dd/yyyy");
                ipo.region = QString("🇺🇸 North America (US)");
                ipo.ticker = ipoObj["proposedTickerSymbol"].toString();
                ipo.status = "filed";

                retrievedIpos.append(ipo);
            }
        }
    }

    if (dataObj["withdrawn"] != QJsonValue::Undefined) {
        QJsonObject withdrawnObj = dataObj["withdrawn"].toObject();

        if (withdrawnObj["rows"] != QJsonValue::Undefined) {
            QJsonArray withdrawnRowsArray = withdrawnObj["rows"].toArray();

            foreach (const QJsonValue &item, withdrawnRowsArray) {
                Ipo ipo;
                QJsonObject ipoObj = item.toObject();

                ipo.company_name = ipoObj["companyName"].toString();
                ipo.expected_date = QDateTime::fromString(ipoObj["withdrawDate"].toString(), "MM/dd/yyyy");
                ipo.region = QString("🇺🇸 North America (US)");
                ipo.stock_exchange = ipoObj["proposedExchange"].toString();
                ipo.ticker = ipoObj["proposedTickerSymbol"].toString();
                ipo.status = "withdrawn";

                retrievedIpos.append(ipo);
            }
        }
    }

    reply->deleteLater();

    return retrievedIpos;
}
