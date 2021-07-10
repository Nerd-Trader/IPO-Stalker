/*
 * IPO Calendar data source: US IPOs
 *
 */

#include <QDate>
#include <QDateTime>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextCodec>
#include <QUrlQuery>

#include "data-sources/nasdaq.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_NASDAQ_DATE_FORMAT     "MM/dd/yyyy"
#define DATA_SOURCE_NASDAQ_DATE_FORMAT_URL "yyyy-MM"
#define DATA_SOURCE_NASDAQ_SOURCE_NAME     "nasdaq.com"

DataSourceNasdaq::DataSourceNasdaq(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_NASDAQ_SOURCE_NAME);
    DataSource::setQueryInterval(6 * 60 * 60);
}

DataSourceNasdaq::~DataSourceNasdaq()
{
}

void DataSourceNasdaq::queryData()
{
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("date", QDate::currentDate().toString(DATA_SOURCE_NASDAQ_DATE_FORMAT_URL));
    QUrl url = QUrl("https://api.nasdaq.com/api/ipo/calendar");
    url.setQuery(urlQuery);

    QNetworkRequest request(url);

    qDebug() << url.toString();

    reply = manager.get(request);

    while (!reply->isFinished()) {
        QCoreApplication::processEvents();
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int status = statusCode.toInt();

    if (status != 200) {
        return;
    }

    QJsonParseError jsonParseError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &jsonParseError);

    if (jsonParseError.error != QJsonParseError::NoError) {
        return;
    }

    QJsonObject jsonRoot = jsonDocument.object();

    if (jsonRoot["data"] == QJsonValue::Undefined) {
        return;
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
                ipo.status = IPO_STATUS_PRICED;
                ipo.priced_date = QDateTime::fromString(ipoObj["pricedDate"].toString(), DATA_SOURCE_NASDAQ_DATE_FORMAT);
                ipo.region = QString("🇺🇸 North America (US)");
                ipo.stock_exchange = ipoObj["proposedExchange"].toString();
                ipo.ticker = ipoObj["proposedTickerSymbol"].toString();

                retrievedIpos->append(ipo);
            }
        }
    }

    if (dataObj["upcoming"] != QJsonValue::Undefined) {
        QJsonObject upcomingObj = dataObj["upcoming"].toObject();

        if (upcomingObj["upcomingTable"] != QJsonValue::Undefined) {
            QJsonObject upcomingTableObj = upcomingObj["upcomingTable"].toObject();

            if (upcomingTableObj["rows"] != QJsonValue::Undefined) {
                QJsonArray upcomingTableRowsArray = upcomingTableObj["rows"].toArray();

                foreach (const QJsonValue &item, upcomingTableRowsArray) {
                    Ipo ipo;
                    QJsonObject ipoObj = item.toObject();

                    ipo.company_name = ipoObj["companyName"].toString();
                    ipo.status = IPO_STATUS_EXPECTED;
                    ipo.expected_date = QDateTime::fromString(ipoObj["expectedPriceDate"].toString(), DATA_SOURCE_NASDAQ_DATE_FORMAT);
                    ipo.region = QString("🇺🇸 North America (US)");
                    ipo.stock_exchange = ipoObj["proposedExchange"].toString();
                    ipo.ticker = ipoObj["proposedTickerSymbol"].toString();

                    retrievedIpos->append(ipo);
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
                ipo.status = IPO_STATUS_FILED;
                ipo.filed_date = QDateTime::fromString(ipoObj["filedDate"].toString(), DATA_SOURCE_NASDAQ_DATE_FORMAT);
                ipo.region = QString("🇺🇸 North America (US)");
                ipo.ticker = ipoObj["proposedTickerSymbol"].toString();

                retrievedIpos->append(ipo);
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
                ipo.status = IPO_STATUS_WITHDRAWN;
                ipo.withdrawn_date = QDateTime::fromString(ipoObj["withdrawDate"].toString(), DATA_SOURCE_NASDAQ_DATE_FORMAT);
                ipo.region = QString("🇺🇸 North America (US)");
                ipo.stock_exchange = ipoObj["proposedExchange"].toString();
                ipo.ticker = ipoObj["proposedTickerSymbol"].toString();

                retrievedIpos->append(ipo);
            }
        }
    }

    reply->deleteLater();
    delete reply;
}
