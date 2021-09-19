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
    IPO_STATUS_EXPECTED,
    IPO_STATUS_PRICED,
    IPO_STATUS_WITHDRAWN,
    IPO_STATUS_FILED,
    IPO_STATUS_UNKNOWN,
};

enum IpoRegion {
    IPO_REGION_COUNTRY_JAPAN,
    IPO_REGION_COUNTRY_USA,
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
