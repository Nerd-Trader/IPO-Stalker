#pragma once

#include <QDateTime>
#include <QStringList>
#include <QUrl>

/*
//                     listed (priced)
//                   /
// filed -> expected
//      \____________\
//                     withdrawn
*/
enum IpoStatus {
    IPO_STATUS_FILED,
    IPO_STATUS_EXPECTED,
    IPO_STATUS_PRICED,
    IPO_STATUS_WITHDRAWN,
    IPO_STATUS_UNKNOWN,
};

enum IpoRegion {
    IPO_REGION_EA_JAPAN,
    IPO_REGION_EU_BELGIUM,
    IPO_REGION_EU_FRANCE,
    IPO_REGION_EU_IRELAND,
    IPO_REGION_EU_ITALY,
    IPO_REGION_EU_NETHERLANDS,
    IPO_REGION_EU_NORWAY,
    IPO_REGION_EU_PORTUGAL,
    IPO_REGION_EU_UK,
    IPO_REGION_NA_CANADA,
    IPO_REGION_NA_USA,
    IPO_REGION_SA_INDIA,
    IPO_REGION_GLOBAL,
    IPO_REGION_UNKNOWN,
};

struct Ipo {
    int         id                = 0;

    bool        is_important      = false;
    int         last_notification = 0;

    QDateTime   filed_date;
    QDateTime   expected_date;
    QDateTime   priced_date;
    QDateTime   withdrawn_date;

    QString     company_name;
    QString     ticker;
    QString     stock_exchange;
    IpoStatus   status            = IPO_STATUS_UNKNOWN;

    QUrl        company_website;
    IpoRegion   region            = IPO_REGION_UNKNOWN;
    QString     market_sector;

    QStringList sources;

    QString     notes;
    QString     meta;
    QString     log;
};
