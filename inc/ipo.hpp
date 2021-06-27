#pragma once

#include <QDateTime>
#include <QStringList>
#include <QUrl>

enum IpoStatus {
    IPO_STATUS_EXPECTED,
    IPO_STATUS_FILED,
    IPO_STATUS_PRICED,
    IPO_STATUS_WITHDRAWN,
    IPO_STATUS_UNKNOWN,
};

struct Ipo {
    QString   company_name;
    QUrl      company_website;

    IpoStatus status; // "filed", "expected", "priced", "withdrawn"
    /*
    //                     priced
    //                   /
    // filed -> expected
    //      \____________\
    //                     withdrawn
    */
    QDateTime filed_date;
    QDateTime expected_date;
    QDateTime priced_date;
    QDateTime withdrawn_date;

    QString   ticker;

    QString   region;
    QString   market_sector;
    QString   stock_exchange;

    QStringList sources;
};
