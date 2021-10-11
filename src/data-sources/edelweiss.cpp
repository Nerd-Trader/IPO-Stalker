/*
 * IPO Stalker data source: Indian IPOs
 *
 */

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextCodec>

#include "data-sources/edelweiss.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_EDELWEISS_DATE_FORMAT Qt::ISODate
#define DATA_SOURCE_EDELWEISS_SOURCE_NAME "edelweiss.in"

DataSourceEdelweiss::DataSourceEdelweiss(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_EDELWEISS_SOURCE_NAME);
    DataSource::setQueryInterval(3 * 60 * 60);
}

DataSourceEdelweiss::~DataSourceEdelweiss()
{
}

void DataSourceEdelweiss::queryData()
{
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    const QUrl url = QUrl("https://ewmw.edelweiss.in/api/ipo/getIPOData");
    QNetworkRequest request(url);

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
            return;
        }

        QJsonObject jsonRoot = jsonDocument.object();

        if (jsonRoot["IPOOngoing"] == QJsonValue::Undefined ||
            jsonRoot["IPOUpcoming"] == QJsonValue::Undefined ||
            jsonRoot["IPOAboutList"] == QJsonValue::Undefined ||
            jsonRoot["IPOListed"] == QJsonValue::Undefined
        ) {
            return;
        }

        QList<Ipo> ipos;

        // Ongoing IPOs
        {
            QJsonArray dataArray = jsonRoot["IPOOngoing"].toArray();

            foreach (const QJsonValue &item, dataArray) {
                Ipo ipo;
                QJsonObject ipoObj = item.toObject();

                ipo.company_name = ipoObj["Name"].toString();
                ipo.status = IPO_STATUS_PRICED;
                ipo.expected_date = QDateTime::fromString(ipoObj["OPENDATE"].toString(), DATA_SOURCE_EDELWEISS_DATE_FORMAT);
                ipo.region = IPO_REGION_SA_INDIA;
                ipo.stock_exchange = ipoObj["EXCHG"].toString().replace("\r\n", " ");
                ipo.ticker = ipoObj["NSE_Symbol"].toString();
                if (ipo.ticker.size() == 0) {
                    ipo.ticker = ipoObj["BSE_Symbol"].toString();
                }

                ipos.append(ipo);
            }
        }

        // Upcoming IPOs
        {
            // TODO
        }

        // About to be Listed IPOs
        {
            QJsonArray dataArray = jsonRoot["IPOAboutList"].toArray();

            foreach (const QJsonValue &item, dataArray) {
                Ipo ipo;
                QJsonObject ipoObj = item.toObject();

                ipo.company_name = ipoObj["Name"].toString();
                ipo.status = IPO_STATUS_EXPECTED;
                ipo.expected_date = QDateTime::fromString(ipoObj["LISTDATE"].toString(), DATA_SOURCE_EDELWEISS_DATE_FORMAT);
                ipo.region = IPO_REGION_SA_INDIA;
                ipo.stock_exchange = ipoObj["EXCHG"].toString().replace("\r\n", " ");
                ipo.ticker = ipoObj["NSE_Symbol"].toString();
                if (ipo.ticker.size() == 0) {
                    ipo.ticker = ipoObj["BSE_Symbol"].toString();
                }

                ipos.append(ipo);
            }
        }

        // Recently Listed IPOs
        {
            QJsonArray dataArray = jsonRoot["IPOListed"].toArray();

            foreach (const QJsonValue &item, dataArray) {
                Ipo ipo;
                QJsonObject ipoObj = item.toObject();

                ipo.company_name = ipoObj["Name"].toString();
                ipo.status = IPO_STATUS_PRICED;
                ipo.priced_date = QDateTime::fromString(ipoObj["LISTDATE"].toString(), DATA_SOURCE_EDELWEISS_DATE_FORMAT);
                ipo.region = IPO_REGION_SA_INDIA;
                ipo.stock_exchange = ipoObj["EXCHG"].toString().replace("\r\n", " ");
                ipo.ticker = ipoObj["NSE_Symbol"].toString();
                if (ipo.ticker.size() == 0) {
                    ipo.ticker = ipoObj["BSE_Symbol"].toString();
                }

                ipos.append(ipo);
            }
        }

        emit ipoInfoObtainedSignal(&ipos);
    }

    reply->deleteLater();
    delete reply;
}
