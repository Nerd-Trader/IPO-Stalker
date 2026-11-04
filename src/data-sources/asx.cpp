/*
 * IPO Stalker data source: Australian IPOs (Australian Securities Exchange)
 *
 * Companies intending to list over the next four to six weeks that have made a formal application:
 * https://www.asx.com.au/listings/upcoming-floats-and-listings
 */

#include <QDate>
#include <QDateTime>
#include <QRegularExpression>

#include "data-sources/asx.hpp"
#include "html.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_ASX_SOURCE_NAME "asx.com.au"
#define DATA_SOURCE_ASX_URL         "https://www.asx.com.au/listings/upcoming-floats-and-listings"

DataSourceAsx::DataSourceAsx(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_ASX_SOURCE_NAME);
    DataSource::setQueryInterval(6 * 60 * 60);
}

DataSourceAsx::~DataSourceAsx()
{
}

QDate DataSourceAsx::parseDate(const QString& str)
{
    // e.g. "Thursday 12 November 2026 12.00pm AEDT"
    static const QRegularExpression dateRe("\\b(\\d{1,2})\\s+([A-Z][a-z]+)\\s+(\\d{4})\\b");
    static const QStringList months = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December",
    };

    const QRegularExpressionMatch m = dateRe.match(str);
    if (!m.hasMatch()) {
        return QDate();
    }

    int month = months.indexOf(m.captured(2)) + 1;
    if (month == 0) {
        // Abbreviated month names
        for (int i = 0; i < months.size(); i++) {
            if (m.captured(2).startsWith(months[i].left(3))) {
                month = i + 1;
                break;
            }
        }
    }

    return QDate(m.captured(3).toInt(), month, m.captured(1).toInt());
}

QList<Ipo> DataSourceAsx::parsePage(const QString& html)
{
    static const QRegularExpression headingRe("<h[2-4]\\b[^>]*>(.*?)</h[2-4]\\s*>",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression nameRe(
        "^(.*?)\\s+[-–]\\s+(?:Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday|Withdrawn|Postponed|TBA|To be advised|\\d)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression domainRe("\\b((?:[a-z0-9-]+\\.)+[a-z]{2,})\\b", QRegularExpression::CaseInsensitiveOption);

    QList<Ipo> ipos;

    // Every company has a heading (name and listing date) followed by a two-column table of details
    QList<QRegularExpressionMatch> headings;
    {
        QRegularExpressionMatchIterator it = headingRe.globalMatch(html);
        while (it.hasNext()) {
            headings << it.next();
        }
    }

    for (int i = 0; i < headings.size(); i++) {
        const int sectionStart = headings[i].capturedEnd();
        const int sectionEnd = (i + 1 < headings.size()) ? headings[i + 1].capturedStart() : html.size();
        const QString section = html.mid(sectionStart, sectionEnd - sectionStart);

        QString listingDate, securityCode, contactDetails, principalActivities;
        for (const QStringList& cells : Html::tableRows(section)) {
            if (cells.size() < 2) {
                continue;
            }
            const QString label = cells[0].toLower();
            if (label.startsWith("listing date")) {
                listingDate = cells[1];
            } else if (label.startsWith("security code") || label.startsWith("asx code")) {
                securityCode = cells[1];
            } else if (label.startsWith("contact details")) {
                contactDetails = cells[1];
            } else if (label.startsWith("principal activities")) {
                principalActivities = cells[1];
            }
        }

        // Not a company section (e.g. an alphabetical group heading)
        if (listingDate.isEmpty()) {
            continue;
        }

        const QString heading = Html::toText(headings[i].captured(1)).remove('#').trimmed();
        const QRegularExpressionMatch nameMatch = nameRe.match(heading);
        Ipo ipo;

        ipo.company_name = nameMatch.hasMatch() ? nameMatch.captured(1).trimmed() : heading;
        if (ipo.company_name.isEmpty()) {
            continue;
        }
        ipo.ticker = securityCode.remove('*').trimmed();
        ipo.region = IPO_REGION_OC_AUSTRALIA;
        ipo.stock_exchange = "ASX";
        if (principalActivities.size() <= 64) {
            ipo.market_sector = principalActivities;
        }
        {
            const QRegularExpressionMatch domainMatch = domainRe.match(contactDetails);
            if (domainMatch.hasMatch()) {
                ipo.company_website = QUrl("https://" + domainMatch.captured(1).toLower());
            }
        }

        const QDate date = parseDate(listingDate);
        if (listingDate.contains("withdrawn", Qt::CaseInsensitive)) {
            ipo.status = IPO_STATUS_WITHDRAWN;
        } else if (date.isValid()) {
            ipo.status = IPO_STATUS_EXPECTED;
            ipo.expected_date = QDateTime(date, QTime(0, 0));
        } else {
            // Formal application made, listing date not known yet
            ipo.status = IPO_STATUS_FILED;
        }

        ipos.append(ipo);
    }

    return ipos;
}

void DataSourceAsx::queryData()
{
    get(QUrl(DATA_SOURCE_ASX_URL), [this](int status, const QByteArray& body) {
        if (status != 200) {
            return;
        }

        const QList<Ipo> ipos = parsePage(QString::fromUtf8(body));
        if (!ipos.isEmpty()) {
            emit ipoInfoObtainedSignal(ipos);
        }
    });
}
