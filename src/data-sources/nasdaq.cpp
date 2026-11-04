/*
 * IPO Stalker data source: US IPOs
 *
 */

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrlQuery>

#include "data-sources/nasdaq.hpp"
#include "ipo.hpp"

// Dates come without zero-padding, e.g. "8/19/2026" (M and d also accept two digits)
#define DATA_SOURCE_NASDAQ_DATE_FORMAT     "M/d/yyyy"
#define DATA_SOURCE_NASDAQ_DATE_FORMAT_URL "yyyy-MM"
#define DATA_SOURCE_NASDAQ_SOURCE_NAME     "nasdaq.com"
#define DATA_SOURCE_NASDAQ_MONTHS          2
#define DATA_SOURCE_NASDAQ_REQUEST_DELAY   5 // Seconds

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
    const QDate date = QDate::currentDate();

    // Space out requests to be polite to the API
    for (int i = 0; i < DATA_SOURCE_NASDAQ_MONTHS; i++) {
        const QDate targetDate = date.addMonths(i);
        QTimer::singleShot(i * DATA_SOURCE_NASDAQ_REQUEST_DELAY * 1000, this, [this, targetDate]() {
            scrapeMonthsData(targetDate);
        });
    }
}

void DataSourceNasdaq::scrapeMonthsData(const QDate& targetDate)
{
    QUrl url = QUrl("https://api.nasdaq.com/api/ipo/calendar");
    {
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("date", targetDate.toString(DATA_SOURCE_NASDAQ_DATE_FORMAT_URL));
        url.setQuery(urlQuery);
    }

    // api.nasdaq.com drops requests which don't look like they come from its own website
    get(url, [this](int status, const QByteArray& body) {
        parseReply(status, body);
    }, {
        { "Accept", "application/json, text/plain, */*" },
        { "Origin", "https://www.nasdaq.com" },
        { "Referer", "https://www.nasdaq.com/" },
    });
}

void DataSourceNasdaq::parseRows(const QJsonArray& rows, const IpoStatus status, QList<Ipo>* ipos)
{
    for (const QJsonValue& item : rows) {
        const QJsonObject ipoObj = item.toObject();
        Ipo ipo;

        ipo.company_name = ipoObj["companyName"].toString().trimmed();
        if (ipo.company_name.isEmpty()) {
            continue;
        }
        ipo.status = status;
        ipo.region = IPO_REGION_NA_USA;
        ipo.stock_exchange = ipoObj["proposedExchange"].toString().trimmed();
        if (ipo.stock_exchange == "N/A") {
            ipo.stock_exchange.clear();
        }
        ipo.ticker = ipoObj["proposedTickerSymbol"].toString();

        switch (status) {
            case IPO_STATUS_FILED:
                ipo.filed_date = QDateTime::fromString(ipoObj["filedDate"].toString(), DATA_SOURCE_NASDAQ_DATE_FORMAT);
                break;
            case IPO_STATUS_EXPECTED:
                ipo.expected_date = QDateTime::fromString(ipoObj["expectedPriceDate"].toString(), DATA_SOURCE_NASDAQ_DATE_FORMAT);
                break;
            case IPO_STATUS_PRICED:
                ipo.priced_date = QDateTime::fromString(ipoObj["pricedDate"].toString(), DATA_SOURCE_NASDAQ_DATE_FORMAT);
                break;
            case IPO_STATUS_WITHDRAWN:
                ipo.withdrawn_date = QDateTime::fromString(ipoObj["withdrawDate"].toString(), DATA_SOURCE_NASDAQ_DATE_FORMAT);
                break;
            default:
                break;
        }

        ipos->append(ipo);
    }
}

void DataSourceNasdaq::parseReply(int status, const QByteArray& body)
{
    if (status != 200) {
        return;
    }

    const QList<Ipo> ipos = parseCalendar(body);

    if (!ipos.isEmpty()) {
        emit ipoInfoObtainedSignal(ipos);
    }
}

QList<Ipo> DataSourceNasdaq::parseCalendar(const QByteArray& json)
{
    QList<Ipo> ipos;
    QJsonParseError jsonParseError;
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(json, &jsonParseError);

    if (jsonParseError.error != QJsonParseError::NoError) {
        return ipos;
    }

    const QJsonObject dataObj = jsonDocument.object()["data"].toObject();

    parseRows(dataObj["priced"].toObject()["rows"].toArray(), IPO_STATUS_PRICED, &ipos);
    parseRows(dataObj["upcoming"].toObject()["upcomingTable"].toObject()["rows"].toArray(), IPO_STATUS_EXPECTED, &ipos);
    parseRows(dataObj["filed"].toObject()["rows"].toArray(), IPO_STATUS_FILED, &ipos);
    parseRows(dataObj["withdrawn"].toObject()["rows"].toArray(), IPO_STATUS_WITHDRAWN, &ipos);

    return ipos;
}
