/*
 * IPO Stalker data source: Japanese IPOs (Tokyo Stock Exchange)
 *
 * Official list of new listings published by Japan Exchange Group:
 * https://www.jpx.co.jp/english/listing/stocks/new/index.html
 */

#include <QDate>
#include <QDateTime>
#include <QRegularExpression>

#include "data-sources/jpx.hpp"
#include "html.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_JPX_SOURCE_NAME "jpx.co.jp"
#define DATA_SOURCE_JPX_URL         "https://www.jpx.co.jp/english/listing/stocks/new/index.html"

DataSourceJpx::DataSourceJpx(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_JPX_SOURCE_NAME);
    DataSource::setQueryInterval(4 * 60 * 60);
}

DataSourceJpx::~DataSourceJpx()
{
}

QList<QDate> DataSourceJpx::parseDates(const QString& str)
{
    // e.g. "Dec. 27, 2024 (Nov. 22, 2024)", "May 14, 2021", "Sept. 1, 2025"
    static const QRegularExpression dateRe("\\b([A-Z][a-z]{2})[a-z]*\\.?\\s+(\\d{1,2}),\\s*(\\d{4})");
    static const QStringList months = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    QList<QDate> dates;
    QRegularExpressionMatchIterator it = dateRe.globalMatch(str);

    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const int month = months.indexOf(m.captured(1)) + 1;
        const QDate date(m.captured(3).toInt(), month, m.captured(2).toInt());
        if (month > 0 && date.isValid()) {
            dates << date;
        }
    }

    return dates;
}

QList<Ipo> DataSourceJpx::parsePage(const QString& html, const QDate& today)
{
    static const QRegularExpression codeRe("^[0-9][0-9A-Z]{2}[0-9A-Z]$");
    static const QRegularExpression footnoteRe("\\s*\\*+$");
    static const QStringList segments = { "Prime", "Standard", "Growth", "TOKYO PRO Market" };

    QList<Ipo> ipos;
    Ipo* current = nullptr;

    // Every company spans two table rows:
    //   1st: Date of Listing (Date of Listing Approval), Issue Name, Code, ...
    //   2nd: Market Segment, ...
    for (const QStringList& cells : Html::tableRows(html)) {
        if (cells.size() >= 3 && codeRe.match(cells[2]).hasMatch()) {
            const QList<QDate> dates = parseDates(cells[0]);
            if (dates.isEmpty() || cells[1].isEmpty()) {
                current = nullptr;
                continue;
            }

            Ipo ipo;
            ipo.company_name = QString(cells[1]).remove(footnoteRe);
            ipo.ticker = cells[2];
            ipo.region = IPO_REGION_EA_JAPAN;
            ipo.stock_exchange = "TSE";

            const QDate listingDate = dates[0];
            if (listingDate > today) {
                ipo.status = IPO_STATUS_EXPECTED;
                ipo.expected_date = QDateTime(listingDate, QTime(0, 0));
            } else {
                ipo.status = IPO_STATUS_PRICED;
                ipo.priced_date = QDateTime(listingDate, QTime(0, 0));
            }
            // Listing approval date
            if (dates.size() > 1) {
                ipo.filed_date = QDateTime(dates[1], QTime(0, 0));
            }

            ipos.append(ipo);
            current = &ipos.last();
        } else if (current != nullptr && !cells.isEmpty()) {
            for (const QString& segment : segments) {
                if (cells[0].startsWith(segment, Qt::CaseInsensitive)) {
                    current->stock_exchange = QString("TSE %1").arg(segment);
                    break;
                }
            }
            current = nullptr;
        }
    }

    return ipos;
}

void DataSourceJpx::queryData()
{
    get(QUrl(DATA_SOURCE_JPX_URL), [this](int status, const QByteArray& body) {
        if (status != 200) {
            return;
        }

        const QList<Ipo> ipos = parsePage(QString::fromUtf8(body), QDate::currentDate());
        if (!ipos.isEmpty()) {
            emit ipoInfoObtainedSignal(ipos);
        }
    });
}
