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
    query.addQueryItem("from", getCurrentDate(-1));
    query.addQueryItem("to", getCurrentDate(3));
    url.setQuery(query.query());
    this->apiKey = apiKey;
}

DataSourceFinnhub::~DataSourceFinnhub()
{
    delete reply;
}

QString DataSourceFinnhub::getCurrentDate(int monthDiff)
{
    QDate now = QDate::currentDate();
    now = now.addMonths(monthDiff);
    return now.toString(DATA_SOURCE_FINNHUB_DATE_FORMAT);
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

    qDebug() << url.toString();

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
            QString statusString = ipoObj["status"].toString();
            if (statusString == "filed") {
                ipo.status = IPO_STATUS_FILED;
            } else if (statusString == "expected") {
                ipo.status = IPO_STATUS_EXPECTED;
            } else if (statusString == "priced") {
                ipo.status = IPO_STATUS_PRICED;
            } else if (statusString == "withdrawn") {
                ipo.status = IPO_STATUS_WITHDRAWN;
            } else {
                ipo.status = IPO_STATUS_UNKNOWN;
            }
            // ipo.company_website = QUrl("https://ddg.gg/?q=" + ipo.company_name);
            QDateTime date = QDateTime::fromString(ipoObj["date"].toString(), DATA_SOURCE_FINNHUB_DATE_FORMAT);
            if (ipo.status == IPO_STATUS_FILED) {
                ipo.filed_date = date;
            } else if (ipo.status == IPO_STATUS_EXPECTED) {
                ipo.expected_date = date;
            } else if (ipo.status == IPO_STATUS_PRICED) {
                ipo.priced_date = date;
            } else if (ipo.status == IPO_STATUS_WITHDRAWN) {
                ipo.withdrawn_date = date;
            }
            ipo.region = QString("🇺🇸 North America (US)");
            ipo.stock_exchange = ipoObj["exchange"].toString();
            ipo.ticker = ipoObj["symbol"].toString();
            ipo.sources << DATA_SOURCE_FINNHUB_SOURCE_NAME;

            retrievedIpos.append(ipo);
        }
    }

    reply->deleteLater();

    return retrievedIpos;
}
