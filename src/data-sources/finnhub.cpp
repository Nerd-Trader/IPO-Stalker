/*
 * IPO Calendar data source: US IPOs
 * API Spec: https://finnhub.io/docs/api
 *
 */

#include <QCalendar>
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
#include <QUrlQuery>

#include "data-sources/finnhub.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_FINNHUB_DATE_FORMAT "yyyy-MM-dd"
#define DATA_SOURCE_FINNHUB_SOURCE_NAME "finnhub.io"
#define DATA_SOURCE_FINNHUB_TIME_ZONE   "ET"

DataSourceFinnhub::DataSourceFinnhub(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_FINNHUB_SOURCE_NAME);
    DataSource::setQueryInterval(1 * 60 * 60);

    settings = new Settings();
}

DataSourceFinnhub::~DataSourceFinnhub()
{
}

QString DataSourceFinnhub::getCurrentDate(int monthDiff)
{
    QDate now = QDate::currentDate();
    now = now.addMonths(monthDiff);
    return now.toString(DATA_SOURCE_FINNHUB_DATE_FORMAT);
}

void DataSourceFinnhub::queryData()
{
    /* No GUI configuration yet, but adding
        [Secrets]
        finnhubApiKey=XyourXsecretXfinnhubXapiXkeyX
       to
        ~/.config/ipo-calendar/ipo-calendar.ini
       will do.
    */
    QString finnhubApiKey;
    if (settings->get("Secrets/finnhubApiKey").size() > 0) {
        finnhubApiKey = settings->get("Secrets/finnhubApiKey");
    }
    QNetworkAccessManager manager;
    QUrlQuery query;
    QNetworkReply *reply;
    QUrl url = QUrl("https://finnhub.io/api/v1/calendar/ipo");

    query.addQueryItem("from", getCurrentDate(-1));
    query.addQueryItem("to", getCurrentDate(3));
    url.setQuery(query.query());

    QNetworkRequest request(url);

    if (finnhubApiKey.isEmpty()) {
        qDebug() << "No finnhub API key set" << DataSource::getName();
        return;
    }

    request.setRawHeader("X-Finnhub-Token", finnhubApiKey.toUtf8());

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
            return;
        }
        QJsonObject jsonRoot = jsonDocument.object();
        if (jsonRoot["ipoCalendar"] == QJsonValue::Undefined) {
            return;
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
            {
                const QString dateTimeStr = ipoObj["date"].toString() + " " + DATA_SOURCE_FINNHUB_TIME_ZONE;
                const QDateTime dateTimeThere = QDateTime::fromString(dateTimeStr, DATA_SOURCE_FINNHUB_DATE_FORMAT" t", QCalendar::QCalendar());
                if (ipo.status == IPO_STATUS_FILED) {
                    ipo.filed_date = dateTimeThere.toLocalTime();
                } else if (ipo.status == IPO_STATUS_EXPECTED) {
                    ipo.expected_date = dateTimeThere.toLocalTime();
                } else if (ipo.status == IPO_STATUS_PRICED) {
                    ipo.priced_date = dateTimeThere.toLocalTime();
                } else if (ipo.status == IPO_STATUS_WITHDRAWN) {
                    ipo.withdrawn_date = dateTimeThere.toLocalTime();
                }
            }
            ipo.region = QString("🇺🇸 North America (US)");
            ipo.stock_exchange = ipoObj["exchange"].toString();
            ipo.ticker = ipoObj["symbol"].toString();

            retrievedIpos->append(ipo);
        }
    }

    reply->deleteLater();
    delete reply;
}
