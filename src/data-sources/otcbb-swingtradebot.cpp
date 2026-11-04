/*
 * IPO Stalker data source: recent OTC IPOs
 *
 */

#include <QDateTime>
#include <QLocale>

#include "data-sources/otcbb-swingtradebot.hpp"
#include "html.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_OTCBBSWINGTRADEBOT_SOURCE_NAME "otcbb.swingtradebot.com"

DataSourceOtcbbSwingtradebot::DataSourceOtcbbSwingtradebot(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_OTCBBSWINGTRADEBOT_SOURCE_NAME);
    DataSource::setQueryInterval(6 * 60 * 60);
}

DataSourceOtcbbSwingtradebot::~DataSourceOtcbbSwingtradebot()
{
}

QList<Ipo> DataSourceOtcbbSwingtradebot::parsePage(const QString& html, const QDate& today)
{
    QList<Ipo> ipos;
    const QStringList tables = Html::elements(html, "table");

    if (tables.isEmpty()) {
        return ipos;
    }

    // Columns get added and rearranged over time, so locate them by their header names
    int tickerCol = 1, nameCol = 2, daysCol = 8, dateCol = -1, exchangeCol = -1;
    {
        const QStringList headerCells = Html::tableRows(Html::elements(tables.first(), "thead").value(0)).value(0);
        for (int i = 0; i < headerCells.size(); i++) {
            const QString header = headerCells[i].toLower();
            if (header == "symbol" || header == "ticker") {
                tickerCol = i;
            } else if (header == "name" || header.contains("company")) {
                nameCol = i;
            } else if (header.contains("days")) {
                daysCol = i;
            } else if (header.contains("date")) {
                dateCol = i;
            } else if (header.contains("exchange")) {
                exchangeCol = i;
            }
        }
    }

    const QString tbody = Html::elements(tables.first(), "tbody").value(0);

    for (const QStringList& cells : Html::tableRows(tbody)) {
        if (cells.size() <= qMax(tickerCol, nameCol) || cells[nameCol].isEmpty()) {
            continue;
        }

        QDate listingDate;
        if (dateCol > -1 && dateCol < cells.size()) {
            for (const char* format : { "yyyy-MM-dd", "M/d/yyyy", "MMM d, yyyy" }) {
                listingDate = QLocale::c().toDate(cells[dateCol], format);
                if (listingDate.isValid()) {
                    break;
                }
            }
        }
        if (!listingDate.isValid() && daysCol > -1 && daysCol < cells.size()) {
            bool ok;
            const int daysOld = cells[daysCol].toInt(&ok);
            if (ok) {
                listingDate = today.addDays(-daysOld);
            }
        }
        if (!listingDate.isValid()) {
            continue;
        }

        Ipo ipo;
        ipo.ticker = cells[tickerCol];
        ipo.company_name = cells[nameCol];
        ipo.stock_exchange = (exchangeCol > -1 && exchangeCol < cells.size()) ? cells[exchangeCol] : "OTC Markets";
        ipo.status = IPO_STATUS_PRICED;
        ipo.priced_date = QDateTime(listingDate, QTime(0, 0));
        ipo.region = IPO_REGION_NA_USA;

        ipos.append(ipo);
    }

    return ipos;
}

void DataSourceOtcbbSwingtradebot::queryData()
{
    get(QUrl("https://otcbb.swingtradebot.com/equities/recent-ipos"), [this](int status, const QByteArray& body) {
        if (status != 200) {
            return;
        }

        const QList<Ipo> ipos = parsePage(QString::fromUtf8(body), QDate::currentDate());
        if (!ipos.isEmpty()) {
            emit ipoInfoObtainedSignal(ipos);
        }
    });
}
