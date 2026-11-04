/*
 * IPO Stalker data source: Indian IPOs (NSE mainboard and NSE Emerge SME platform)
 *
 * Uses the same JSON endpoints as https://www.nseindia.com/market-data/all-upcoming-issues-ipo
 * The API only answers to clients that carry the cookies set by the website, hence the website
 * gets visited first.
 */

#include <QDate>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <QUrlQuery>

#include "data-sources/nseindia.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_NSEINDIA_SOURCE_NAME   "nseindia.com"
#define DATA_SOURCE_NSEINDIA_BASE_URL      "https://www.nseindia.com"
#define DATA_SOURCE_NSEINDIA_REFERER       DATA_SOURCE_NSEINDIA_BASE_URL "/market-data/all-upcoming-issues-ipo"
#define DATA_SOURCE_NSEINDIA_DATE_FORMAT_URL "dd-MM-yyyy"
#define DATA_SOURCE_NSEINDIA_REQUEST_DELAY 2 // Seconds

DataSourceNseIndia::DataSourceNseIndia(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_NSEINDIA_SOURCE_NAME);
    DataSource::setQueryInterval(3 * 60 * 60);
}

DataSourceNseIndia::~DataSourceNseIndia()
{
}

QDate DataSourceNseIndia::parseDate(const QString& str)
{
    // e.g. "26-Mar-2024" or "22-MAR-2024"
    const QStringList parts = str.trimmed().split('-');

    if (parts.size() != 3 || parts[1].size() != 3) {
        return QDate();
    }

    const QString month = parts[1].left(1).toUpper() + parts[1].mid(1).toLower();

    return QLocale::c().toDate(parts[0] + "-" + month + "-" + parts[2], "d-MMM-yyyy");
}

QList<Ipo> DataSourceNseIndia::parseIssues(const QByteArray& json, bool pastIssues)
{
    QList<Ipo> ipos;
    QJsonParseError jsonParseError;
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(json, &jsonParseError);

    if (jsonParseError.error != QJsonParseError::NoError) {
        return ipos;
    }

    // Some endpoints wrap the list into {"data": [...]}
    QJsonArray items = jsonDocument.array();
    if (jsonDocument.isObject()) {
        items = jsonDocument.object()["data"].toArray();
    }

    for (const QJsonValue& item : items) {
        const QJsonObject ipoObj = item.toObject();
        Ipo ipo;

        ipo.company_name = ipoObj["companyName"].toString().trimmed();
        if (ipo.company_name.isEmpty()) {
            ipo.company_name = ipoObj["company"].toString().trimmed();
        }
        if (ipo.company_name.isEmpty()) {
            continue;
        }

        ipo.ticker = ipoObj["symbol"].toString().trimmed();
        ipo.region = IPO_REGION_SA_INDIA;

        const QString series = (ipoObj["series"].toString() + ipoObj["securityType"].toString()).toUpper();
        ipo.stock_exchange = series.contains("SME") ? "NSE Emerge" : "NSE";

        // Book-building closes (and the issue gets priced) at the end of the subscription period
        const QDate closeDate = parseDate(pastIssues ? ipoObj["ipoEndDate"].toString() : ipoObj["issueEndDate"].toString());
        const QDate listingDate = parseDate(ipoObj["listingDate"].toString());

        if (listingDate.isValid()) {
            ipo.status = IPO_STATUS_PRICED;
            ipo.priced_date = QDateTime(listingDate, QTime(0, 0));
        } else if (closeDate.isValid()) {
            ipo.status = IPO_STATUS_EXPECTED;
            ipo.expected_date = QDateTime(closeDate, QTime(0, 0));
        } else {
            continue;
        }

        ipos.append(ipo);
    }

    return ipos;
}

void DataSourceNseIndia::queryData()
{
    // Obtain session cookies first (stored in the network access manager's cookie jar)
    get(QUrl(DATA_SOURCE_NSEINDIA_BASE_URL "/"), [this](int status, const QByteArray& body) {
        (void)body;

        if (status != 200) {
            return;
        }

        queryApi();
    }, {
        { "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8" },
    });
}

void DataSourceNseIndia::queryApi()
{
    const Headers headers = {
        { "Accept", "application/json, text/plain, */*" },
        { "Referer", DATA_SOURCE_NSEINDIA_REFERER },
    };

    QUrl pastIssuesUrl(DATA_SOURCE_NSEINDIA_BASE_URL "/api/public-past-issues");
    {
        const QDate today = QDate::currentDate();
        QUrlQuery query;
        query.addQueryItem("from_date", today.addDays(-30).toString(DATA_SOURCE_NSEINDIA_DATE_FORMAT_URL));
        query.addQueryItem("to_date", today.toString(DATA_SOURCE_NSEINDIA_DATE_FORMAT_URL));
        query.addQueryItem("security_type", "all");
        pastIssuesUrl.setQuery(query);
    }

    const struct { QUrl url; bool past; } endpoints[] = {
        { QUrl(DATA_SOURCE_NSEINDIA_BASE_URL "/api/ipo-current-issue"), false },
        { QUrl(DATA_SOURCE_NSEINDIA_BASE_URL "/api/all-upcoming-issues?category=ipo"), false },
        { pastIssuesUrl, true },
    };

    int i = 0;
    for (const auto& endpoint : endpoints) {
        const QUrl url = endpoint.url;
        const bool past = endpoint.past;

        QTimer::singleShot(i++ * DATA_SOURCE_NSEINDIA_REQUEST_DELAY * 1000, this, [this, url, past, headers]() {
            get(url, [this, past](int status, const QByteArray& body) {
                if (status != 200) {
                    return;
                }

                const QList<Ipo> ipos = parseIssues(body, past);
                if (!ipos.isEmpty()) {
                    emit ipoInfoObtainedSignal(ipos);
                }
            }, headers);
        });
    }
}
