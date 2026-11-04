/*
 * Offline tests for data source parsers and record merging.
 * Fixtures mimic the structure of the real pages/API responses.
 *
 * Run with: ctest --test-dir build   (or ./build/ipo-stalker-tests)
 */

#include <QCoreApplication>
#include <QDate>
#include <QDebug>

#include "data-sources/asx.hpp"
#include "data-sources/euronext.hpp"
#include "data-sources/jpx.hpp"
#include "data-sources/nasdaq.hpp"
#include "data-sources/nseindia.hpp"
#include "data-sources/otcbb-swingtradebot.hpp"
#include "db.hpp"
#include "html.hpp"

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        qWarning().noquote() << QString("FAIL %1:%2: %3").arg(__FILE__).arg(__LINE__).arg(#cond); \
    } \
} while (0)

#define CHECK_EQ(a, b) do { \
    checks++; \
    const auto va = (a); const auto vb = (b); \
    if (!(va == vb)) { \
        failures++; \
        qWarning().noquote() << QString("FAIL %1:%2: %3 == %4").arg(__FILE__).arg(__LINE__).arg(#a, #b) << va << vb; \
    } \
} while (0)

static void testHtml()
{
    CHECK_EQ(Html::decodeEntities("A &amp; B &#39;x&#x27; &lt;&gt;"), QString("A & B 'x' <>"));
    CHECK_EQ(Html::toText("<b>Foo</b>&nbsp; <i>Bar</i>\n  Baz<br/>Qux"), QString("Foo Bar Baz Qux"));

    const QList<QStringList> rows = Html::tableRows(
        "<table><tr><th>A</th><th>B</th></tr>"
        "<tr class=\"x\"><td>1</td><td><a href=\"#\">two</a></td></tr>"
        "<tr><td>3<td>4</tr></table>"
    );
    CHECK_EQ(rows.size(), 3);
    CHECK_EQ(rows.value(0), QStringList({ "A", "B" }));
    CHECK_EQ(rows.value(1), QStringList({ "1", "two" }));
    CHECK_EQ(rows.value(2), QStringList({ "3", "4" })); // unclosed cells
}

static void testNasdaq()
{
    const QByteArray json = R"({
        "data": {
            "priced": {"rows": [{"proposedTickerSymbol": "LYNX", "companyName": "Lynx Corp.", "proposedExchange": "NASDAQ Global", "pricedDate": "8/19/2026"}]},
            "upcoming": {"upcomingTable": {"rows": [{"proposedTickerSymbol": "PTT", "companyName": "PTT Inc", "proposedExchange": "NYSE", "expectedPriceDate": "9/08/2026"}]}},
            "filed": {"rows": [{"proposedTickerSymbol": "FLD", "companyName": "Filer Ltd", "filedDate": "10/1/2026"}]},
            "withdrawn": {"rows": [{"proposedTickerSymbol": "WDR", "companyName": "Gone Co", "proposedExchange": "N/A", "withdrawDate": "12/31/2026"}]}
        }
    })";

    const QList<Ipo> ipos = DataSourceNasdaq::parseCalendar(json);
    CHECK_EQ(ipos.size(), 4);
    if (ipos.size() != 4) {
        return;
    }
    CHECK_EQ(ipos[0].status, IPO_STATUS_PRICED);
    CHECK_EQ(ipos[0].priced_date.date(), QDate(2026, 8, 19));
    CHECK_EQ(ipos[0].ticker, QString("LYNX"));
    CHECK_EQ(ipos[1].expected_date.date(), QDate(2026, 9, 8));
    CHECK_EQ(ipos[2].filed_date.date(), QDate(2026, 10, 1));
    CHECK_EQ(ipos[3].withdrawn_date.date(), QDate(2026, 12, 31));
    CHECK(ipos[3].stock_exchange.isEmpty());

    CHECK(DataSourceNasdaq::parseCalendar("{\"data\": null}").isEmpty());
    CHECK(DataSourceNasdaq::parseCalendar("garbage").isEmpty());
}

static void testEuronext()
{
    const QString html =
        "<table class=\"table\"><thead><tr><th>Date</th><th>Company name</th><th>Ticker</th>"
        "<th>ISIN code</th><th>Location</th><th>Market</th></tr></thead><tbody>"
        "<tr><td>12/11/2026</td><td><a href=\"/en/ipo-showcase/future-co\">Future Co</a></td><td>FUT</td>"
        "<td>FR0000000001</td><td>Paris</td><td>Euronext Growth</td></tr>"
        "<tr><td>28/10/2026</td><td><a href=\"/en/ipo-showcase/warehouses-pauw-sa\">Warehouses De Pauw SA</a></td>"
        "<td>WDP</td><td>BE0974349814</td><td>Paris</td><td>Euronext</td></tr>"
        "<tr><td>24/09/2026</td><td><a href=\"#\">Outlet Group Holding ASA</a></td><td>OUTLT</td>"
        "<td>NO0013757500</td><td>Oslo</td><td>Euronext Oslo B&oslash;rs</td></tr>"
        "</tbody></table>";

    const QList<Ipo> ipos = DataSourceEuronext::parsePage(html, QDate(2026, 11, 3));
    CHECK_EQ(ipos.size(), 3);
    if (ipos.size() != 3) {
        return;
    }
    CHECK_EQ(ipos[0].status, IPO_STATUS_EXPECTED);
    CHECK_EQ(ipos[0].expected_date.date(), QDate(2026, 11, 12));
    CHECK_EQ(ipos[0].stock_exchange, QString("Euronext Growth (Paris)"));
    CHECK_EQ(ipos[1].status, IPO_STATUS_PRICED);
    CHECK_EQ(ipos[1].priced_date.date(), QDate(2026, 10, 28));
    CHECK_EQ(ipos[1].region, IPO_REGION_EU_FRANCE);
    CHECK_EQ(ipos[2].region, IPO_REGION_EU_NORWAY);
    CHECK_EQ(ipos[2].stock_exchange, QString("Euronext Oslo Børs"));
}

static void testJpx()
{
    const QString html =
        "<table><tr><th>Date of Listing<br>(Date of Listing Approval)</th><th>Issue Name</th><th>Code</th>"
        "<th>Outline</th><th>Written Confirmation</th><th>Indicated Range (JPY)</th><th>Public Offering</th><th>Trading Unit</th></tr>"
        "<tr><th>Market Segment</th><th>Securities Report</th><th>CG Report</th><th>Offering Price</th><th>Secondary Distribution</th><th>Earnings Report</th></tr>"
        "<tr><td rowspan=\"2\">Dec. 27, 2026<br>(Nov. 22, 2026)</td><td rowspan=\"2\">b-style holdings,Inc.</td><td rowspan=\"2\">302A</td>"
        "<td><a href=\"x-OutlineEN.pdf\"><img src=\"pdf.gif\"></a></td><td><a href=\"k.pdf\"><img></a></td><td>1,980-2,070</td><td>200</td><td>100</td></tr>"
        "<tr><td>Growth</td><td><a href=\"1s.pdf\"><img></a></td><td></td><td>2,070</td><td>370(OA85.5)</td><td></td></tr>"
        "<tr><td>Oct. 01, 2026 (Sep. 01, 2026)</td><td>NISSO HOLDINGS Co.,Ltd. **</td><td>9332</td><td></td><td></td><td>-</td><td>-</td><td>100</td></tr>"
        "<tr><td>Prime</td><td></td><td></td><td>-</td><td>-</td><td></td></tr>"
        "</table>";

    const QList<Ipo> ipos = DataSourceJpx::parsePage(html, QDate(2026, 11, 3));
    CHECK_EQ(ipos.size(), 2);
    if (ipos.size() != 2) {
        return;
    }
    CHECK_EQ(ipos[0].company_name, QString("b-style holdings,Inc."));
    CHECK_EQ(ipos[0].ticker, QString("302A"));
    CHECK_EQ(ipos[0].status, IPO_STATUS_EXPECTED);
    CHECK_EQ(ipos[0].expected_date.date(), QDate(2026, 12, 27));
    CHECK_EQ(ipos[0].filed_date.date(), QDate(2026, 11, 22));
    CHECK_EQ(ipos[0].stock_exchange, QString("TSE Growth"));
    CHECK_EQ(ipos[1].company_name, QString("NISSO HOLDINGS Co.,Ltd."));
    CHECK_EQ(ipos[1].status, IPO_STATUS_PRICED);
    CHECK_EQ(ipos[1].priced_date.date(), QDate(2026, 10, 1));
    CHECK_EQ(ipos[1].stock_exchange, QString("TSE Prime"));

    CHECK_EQ(DataSourceJpx::parseDates("May 14, 2021").value(0), QDate(2021, 5, 14));
    CHECK_EQ(DataSourceJpx::parseDates("Sept. 1, 2025").value(0), QDate(2025, 9, 1));
}

static void testAsx()
{
    const QString html =
        "<h3>A-C</h3>"
        "<h3>Capability X Limited - Thursday 12 November 2026 12.00pm AEDT</h3>"
        "<table><tbody>"
        "<tr><td><strong>Listing date</strong></td><td>Thursday 12 November 2026 12.00pm AEDT</td></tr>"
        "<tr><td><strong>Contact details</strong></td><td><a href=\"http://capabilityx.com.au\">capabilityx.com.au</a> Ph: +61 8 9426 0666</td></tr>"
        "<tr><td><strong>Principal activities</strong></td><td>Mining exploration (uranium)</td></tr>"
        "<tr><td><strong>Issue price</strong></td><td>$0.20 AUD</td></tr>"
        "<tr><td><strong>Security code</strong></td><td>CX8</td></tr>"
        "</tbody></table>"
        "<h3>Super Minerals Ltd. - Withdrawn</h3>"
        "<table><tr><td>Listing date</td><td>Withdrawn</td></tr><tr><td>Security code</td><td>SUP*</td></tr></table>"
        "<h3>Rio De Oro Limited - TBA</h3>"
        "<table><tr><td>Listing date</td><td>TBA</td></tr><tr><td>Security code</td><td>RDO</td></tr></table>";

    const QList<Ipo> ipos = DataSourceAsx::parsePage(html);
    CHECK_EQ(ipos.size(), 3);
    if (ipos.size() != 3) {
        return;
    }
    CHECK_EQ(ipos[0].company_name, QString("Capability X Limited"));
    CHECK_EQ(ipos[0].ticker, QString("CX8"));
    CHECK_EQ(ipos[0].expected_date.date(), QDate(2026, 11, 12));
    CHECK_EQ(ipos[0].company_website, QUrl("https://capabilityx.com.au"));
    CHECK_EQ(ipos[0].market_sector, QString("Mining exploration (uranium)"));
    CHECK_EQ(ipos[0].region, IPO_REGION_OC_AUSTRALIA);
    CHECK_EQ(ipos[1].company_name, QString("Super Minerals Ltd."));
    CHECK_EQ(ipos[1].status, IPO_STATUS_WITHDRAWN);
    CHECK_EQ(ipos[1].ticker, QString("SUP"));
    CHECK_EQ(ipos[2].status, IPO_STATUS_FILED);
}

static void testNseIndia()
{
    const QByteArray current = R"([{"symbol": "SRM", "companyName": "SRM Contractors Limited", "series": "EQ",
        "issueStartDate": "26-Mar-2024", "issueEndDate": "28-Mar-2024", "status": "Active"}])";
    const QByteArray past = R"([{"symbol": "NAMAN", "companyName": "Naman In-Store (India) Limited",
        "ipoStartDate": "22-MAR-2024", "ipoEndDate": "27-MAR-2024", "listingDate": "03-APR-2024", "securityType": "SME"},
        {"symbol": "NOLIST", "company": "Not Listed Yet Ltd", "ipoEndDate": "27-MAR-2024", "listingDate": "-", "securityType": "EQ"}])";

    QList<Ipo> ipos = DataSourceNseIndia::parseIssues(current, false);
    CHECK_EQ(ipos.size(), 1);
    if (ipos.size() == 1) {
        CHECK_EQ(ipos[0].status, IPO_STATUS_EXPECTED);
        CHECK_EQ(ipos[0].expected_date.date(), QDate(2024, 3, 28));
        CHECK_EQ(ipos[0].stock_exchange, QString("NSE"));
    }

    ipos = DataSourceNseIndia::parseIssues(past, true);
    CHECK_EQ(ipos.size(), 2);
    if (ipos.size() == 2) {
        CHECK_EQ(ipos[0].status, IPO_STATUS_PRICED);
        CHECK_EQ(ipos[0].priced_date.date(), QDate(2024, 4, 3));
        CHECK_EQ(ipos[0].stock_exchange, QString("NSE Emerge"));
        CHECK_EQ(ipos[1].company_name, QString("Not Listed Yet Ltd"));
        CHECK_EQ(ipos[1].status, IPO_STATUS_EXPECTED);
    }
}

static void testSwingtradebot()
{
    const QString html =
        "<table><thead><tr><th></th><th>Symbol</th><th>Name</th><th>IPO Price</th><th>Price</th>"
        "<th>Change</th><th>Volume</th><th>Exchange</th><th>Days Since IPO</th></tr></thead><tbody>"
        "<tr><td>1</td><td>ABCD</td><td>Alphabet Soup Corp</td><td>4.00</td><td>5.10</td><td>+2%</td>"
        "<td>100,000</td><td>OTC</td><td>3</td></tr></tbody></table>";

    const QList<Ipo> ipos = DataSourceOtcbbSwingtradebot::parsePage(html, QDate(2026, 11, 3));
    CHECK_EQ(ipos.size(), 1);
    if (ipos.size() == 1) {
        CHECK_EQ(ipos[0].ticker, QString("ABCD"));
        CHECK_EQ(ipos[0].company_name, QString("Alphabet Soup Corp"));
        CHECK_EQ(ipos[0].priced_date, QDateTime(QDate(2026, 10, 31), QTime(0, 0)));
        CHECK_EQ(ipos[0].stock_exchange, QString("OTC"));
    }
}

static void testMerge()
{
    CHECK_EQ(Db::normalizeCompanyName("Foo Holdings, Inc."), QString("foo"));
    CHECK_EQ(Db::normalizeCompanyName("FOO HOLDINGS INC"), QString("foo"));
    CHECK_EQ(Db::normalizeCompanyName("Warehouses De Pauw S.A."), QString("warehouses de pauw"));
    CHECK_EQ(Db::normalizeCompanyName("（株）はてな"), QString("はてな"));
    CHECK(Db::normalizeCompanyName("Group") == QString("group")); // never normalize down to nothing

    Ipo a;
    a.company_name = "Acme Corp.";
    a.region = IPO_REGION_NA_USA;
    Ipo b;
    b.company_name = "ACME CORPORATION";
    b.region = IPO_REGION_NA_USA;
    CHECK(Db::checkIfSameIpo(&a, &b));
    b.region = IPO_REGION_EA_JAPAN;
    CHECK(!Db::checkIfSameIpo(&a, &b));
    Ipo c;
    c.company_name = "Totally Different Name";
    c.ticker = "ACME";
    c.region = IPO_REGION_NA_USA;
    a.ticker = "acme";
    CHECK(Db::checkIfSameIpo(&a, &c));

    // A source that knows only the priced date must not erase the filed date
    Ipo existing;
    existing.company_name = "Acme";
    existing.status = IPO_STATUS_EXPECTED;
    existing.filed_date = QDateTime(QDate(2026, 9, 1), QTime(0, 0));
    existing.expected_date = QDateTime(QDate(2026, 10, 1), QTime(0, 0));
    existing.sources << "a";

    Ipo priced;
    priced.company_name = "Acme";
    priced.status = IPO_STATUS_PRICED;
    priced.priced_date = QDateTime(QDate(2026, 10, 2), QTime(0, 0));

    CHECK(Db::mergeIpo(&existing, &priced, "b"));
    CHECK_EQ(existing.filed_date.date(), QDate(2026, 9, 1));
    CHECK_EQ(existing.expected_date.date(), QDate(2026, 10, 1));
    CHECK_EQ(existing.priced_date.date(), QDate(2026, 10, 2));
    CHECK_EQ(existing.status, IPO_STATUS_PRICED);
    CHECK_EQ(existing.sources, QStringList({ "a", "b" }));

    // Status never moves backwards
    Ipo expectedAgain;
    expectedAgain.company_name = "Acme";
    expectedAgain.status = IPO_STATUS_EXPECTED;
    expectedAgain.expected_date = existing.expected_date;
    CHECK(!Db::mergeIpo(&existing, &expectedAgain, "a"));
    CHECK_EQ(existing.status, IPO_STATUS_PRICED);

    // Withdrawal wins
    Ipo withdrawn;
    withdrawn.company_name = "Acme";
    withdrawn.status = IPO_STATUS_WITHDRAWN;
    CHECK(Db::mergeIpo(&existing, &withdrawn, "a"));
    CHECK_EQ(existing.status, IPO_STATUS_WITHDRAWN);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    testHtml();
    testNasdaq();
    testEuronext();
    testJpx();
    testAsx();
    testNseIndia();
    testSwingtradebot();
    testMerge();

    qInfo().noquote() << QString("%1 checks, %2 failure(s)").arg(checks).arg(failures);

    return failures == 0 ? 0 : 1;
}
