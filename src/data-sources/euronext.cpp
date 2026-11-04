/*
 * IPO Stalker data source: European IPOs
 *
 */

#include <QDate>
#include <QDateTime>
#include <QUrlQuery>

#include "data-sources/euronext.hpp"
#include "html.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_EURONEXT_DATE_FORMAT   "dd/MM/yyyy"
#define DATA_SOURCE_EURONEXT_SOURCE_NAME   "euronext.com"
#define DATA_SOURCE_EURONEXT_PAGES         2 // 20 IPOs per page, most recent and upcoming first
#define DATA_SOURCE_EURONEXT_REQUEST_DELAY 3 // Seconds

DataSourceEuronext::DataSourceEuronext(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_EURONEXT_SOURCE_NAME);
    DataSource::setQueryInterval(8 * 60 * 60);
}

DataSourceEuronext::~DataSourceEuronext()
{
}

QList<Ipo> DataSourceEuronext::parsePage(const QString& html, const QDate& today)
{
    QList<Ipo> ipos;

    for (const QStringList& cells : Html::tableRows(html)) {
        // Date, Company name, Ticker, ISIN code, Location, Market
        if (cells.size() < 6) {
            continue;
        }

        const QDate date = QDate::fromString(cells[0], DATA_SOURCE_EURONEXT_DATE_FORMAT);
        if (!date.isValid() || cells[1].isEmpty()) {
            continue;
        }

        Ipo ipo;
        ipo.company_name = cells[1];
        ipo.ticker = cells[2];
        if (date > today) {
            ipo.status = IPO_STATUS_EXPECTED;
            ipo.expected_date = QDateTime(date, QTime(0, 0));
        } else {
            ipo.status = IPO_STATUS_PRICED;
            ipo.priced_date = QDateTime(date, QTime(0, 0));
        }
        {
            const QList<IpoRegion> ipoRegions = tradingLocationsToIpoRegions(cells[4]);
            if (ipoRegions.size() > 0) {
                ipo.region = ipoRegions[0];
                // TODO: make ipo.region a QList, keep all regions there
            }
        }
        ipo.stock_exchange = cells[5];
        if (!cells[4].isEmpty() && !ipo.stock_exchange.contains(cells[4])) {
            ipo.stock_exchange += QString(" (%1)").arg(cells[4]);
        }

        ipos.append(ipo);
    }

    return ipos;
}

void DataSourceEuronext::queryData()
{
    for (int page = 0; page < DATA_SOURCE_EURONEXT_PAGES; page++) {
        QTimer::singleShot(page * DATA_SOURCE_EURONEXT_REQUEST_DELAY * 1000, this, [this, page]() {
            QUrl url = QUrl("https://live.euronext.com/en/ipo-showcase");
            if (page > 0) {
                QUrlQuery urlQuery;
                urlQuery.addQueryItem("page", QString::number(page));
                url.setQuery(urlQuery);
            }

            get(url, [this](int status, const QByteArray& body) {
                if (status != 200) {
                    return;
                }

                const QList<Ipo> ipos = parsePage(QString::fromUtf8(body), QDate::currentDate());
                if (!ipos.isEmpty()) {
                    emit ipoInfoObtainedSignal(ipos);
                }
            });
        });
    }
}

QList<IpoRegion> DataSourceEuronext::tradingLocationsToIpoRegions(const QString& tradingLocations)
{
    QList<IpoRegion> result;
    const QStringList locationsList = tradingLocations.split(",");

    for (const QString& location : locationsList) {
        const QString str = location.trimmed();

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
