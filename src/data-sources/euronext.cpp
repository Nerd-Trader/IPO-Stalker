/*
 * IPO Stalker data source: European IPOs
 *
 */

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrlQuery>
#include <QWebEngineScript>
#include <QWebEngineSettings>

#include "data-sources/euronext.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_EURONEXT_DATE_FORMAT     "dd/MM/yyyy"
#define DATA_SOURCE_EURONEXT_DATE_FORMAT_URL "MM/dd/yyyy"
#define DATA_SOURCE_EURONEXT_SOURCE_NAME     "euronext.com"

DataSourceEuronext::DataSourceEuronext(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_EURONEXT_SOURCE_NAME);
    DataSource::setQueryInterval(8 * 60 * 60);
}

DataSourceEuronext::~DataSourceEuronext()
{
}

void DataSourceEuronext::parseMainPage(QWebEnginePage *page)
{
    page->runJavaScript(" \
        const items = []; \
        const tbodyEl = document.getElementsByTagName('TBODY')[0]; \
        const rowEls = tbodyEl.getElementsByTagName('TR'); \
        for (let i = 0, ilen = rowEls.length; i < ilen; i++) { \
            const item = {}; \
            const colEls = rowEls[i].getElementsByTagName('TD'); \
            for (let j = 0, jlen = colEls.length; j < jlen; j++) { \
                if (j == 0) { \
                    item.date = colEls[j].textContent.trim(); \
                } else if (j == 1) { \
                    item.company_name = colEls[j].textContent.trim(); \
                } else if (j == 2) { \
                    item.ticker = colEls[j].textContent.trim(); \
                } else if (j == 4) { \
                    item.cities = colEls[j].textContent.trim(); \
                } else if (j == 5) { \
                    item.marketplace = colEls[j].textContent.trim(); \
                } \
            } \
            items.push(item); \
        } \
        JSON.stringify(items); \
    ",
    QWebEngineScript::ApplicationWorld,
    [this](const QVariant &items) {
        QJsonParseError jsonParseError;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(items.toByteArray(), &jsonParseError);

        QList<Ipo> ipos;
        foreach (const QJsonValue &item, jsonDocument.array()) {
            Ipo ipo;
            QJsonObject ipoJsonObj = item.toObject();
            ipo.company_name = ipoJsonObj["company_name"].toString();
            ipo.status = IPO_STATUS_EXPECTED;
            ipo.expected_date = QDateTime::fromString(ipoJsonObj["date"].toString(), DATA_SOURCE_EURONEXT_DATE_FORMAT);
            {
                QList<IpoRegion> ipoRegions = tradingLocationsToIpoRegions(ipoJsonObj["cities"].toString());
                if (ipoRegions.size() > 0) {
                    ipo.region = ipoRegions[0];
                    // TODO: make ipo.region a QList, keep all regions there
                }
            }
            ipo.stock_exchange = ipoJsonObj["marketplace"].toString();
            ipo.ticker = ipoJsonObj["ticker"].toString();

            ipos.append(ipo);
        }
        emit ipoInfoObtainedSignal(&ipos);
    });
}

void DataSourceEuronext::queryData()
{
    QUrl url = QUrl("https://live.euronext.com/en/ipo-showcase/all");
    {
        QUrlQuery urlQuery;
        QDate date_min = QDate::currentDate();
        QDate date_max = QDate::currentDate().addMonths(3);
        urlQuery.addQueryItem("field_iponi_ipo_date_value[min]", date_min.toString(DATA_SOURCE_EURONEXT_DATE_FORMAT_URL));
        urlQuery.addQueryItem("field_iponi_ipo_date_value[max]", date_max.toString(DATA_SOURCE_EURONEXT_DATE_FORMAT_URL));
        url.setQuery(urlQuery);
    }

    QWebEnginePage *page = new QWebEnginePage(this);
    // No need to load images
    page->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, false);
    // No need to use JS
    page->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    page->load(url);
    connect(page, &QWebEnginePage::loadFinished, this, [this, page] {
        parseMainPage(page);
    });
}

QList<IpoRegion> DataSourceEuronext::tradingLocationsToIpoRegions(const QString tradingLocations)
{
    QList<IpoRegion> result;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStringList locationsList = tradingLocations.split(", ", Qt::SkipEmptyParts);
#else
    QStringList locationsList = tradingLocations.split(", ", QString::SkipEmptyParts);
#endif

    foreach (const QString &str, locationsList) {
        if (str == "Paris") {
            result << IPO_REGION_EU_FRANCE;
        } else if (str == "Amsterdam") {
            result << IPO_REGION_EU_NETHERLANDS;
        } else if (str == "Oslo") {
            result << IPO_REGION_EU_NORWAY;
        } else if (str == "Brussels") {
            result << IPO_REGION_EU_BELGIUM;
        } else if (str == "Lisbon") {
            result << IPO_REGION_EU_PORTUGAL;
        } else if (str == "Dublin") {
            result << IPO_REGION_EU_IRELAND;
        } else if (str == "London") {
            result << IPO_REGION_EU_UK;
        } else if (str == "Milan") {
            result << IPO_REGION_EU_ITALY;
        }
    }

    return result;
}
