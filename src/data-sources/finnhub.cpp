/*
 * IPO Stalker data source: US IPOs
 * API Spec: https://finnhub.io/docs/api
 *
 */

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrlQuery>

#include "data-sources/finnhub.hpp"
#include "ipo.hpp"

#ifdef DEBUG
#include <QDebug>
#endif

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
    delete settings;
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
        finnhubApiKey=your_secret_finnhub_api_key
       to
        ~/.config/ipo-stalker/ipo-stalker.ini
       will do the job.
    */
    QString finnhubApiKey;
    if (settings->get("Secrets/finnhubApiKey").size() > 0) {
        finnhubApiKey = settings->get("Secrets/finnhubApiKey");
    }
    QUrlQuery query;
    QUrl url = QUrl("https://finnhub.io/api/v1/calendar/ipo");

    query.addQueryItem("from", getCurrentDate(-1));
    query.addQueryItem("to", getCurrentDate(3));
    url.setQuery(query);

    if (finnhubApiKey.isEmpty()) {
#ifdef DEBUG
        qDebug().noquote() << "No API key found for" << *getName();
#endif
        return;
    }

    get(url, [this](int status, const QByteArray& body) {
        parseReply(status, body);
    }, { { "X-Finnhub-Token", finnhubApiKey.toUtf8() } });
}

void DataSourceFinnhub::parseReply(int status, const QByteArray& body)
{
    if (status == 200) {
        QJsonParseError jsonParseError;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(body, &jsonParseError);
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
            ipo.region = IPO_REGION_NA_USA;
            ipo.stock_exchange = ipoObj["exchange"].toString();
            ipo.ticker = ipoObj["symbol"].toString();

            ipos.append(ipo);
        }
        emit ipoInfoObtainedSignal(ipos);
    }
}
