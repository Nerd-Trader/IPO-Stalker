/*
 * IPO Calendar data source: US IPOs
 * API Spec: https://finnhub.io/docs/api
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
#include <QUrlQuery>

#include "data-sources/finnhub.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_FINNHUB_DATE_FORMAT "yyyy-MM-dd"
#define DATA_SOURCE_FINNHUB_SOURCE_NAME "finnhub.io"

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
#ifdef DEBUG
        qDebug().noquote() << "No API key found for" << getName();
#endif
        return;
    }

    request.setRawHeader("X-Finnhub-Token", finnhubApiKey.toUtf8());

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

        QList<Ipo> ipos;
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
            ipo.region = IPO_REGION_COUNTRY_USA;
            ipo.stock_exchange = ipoObj["exchange"].toString();
            ipo.ticker = ipoObj["symbol"].toString();

            ipos.append(ipo);
        }
        emit ipoInfoObtained(&ipos, getName());
    }

    reply->deleteLater();
    delete reply;
}
