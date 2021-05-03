#pragma once

#include <QDateTime>
#include <QUrl>

struct Ipo {
    QString   company_name;
    QUrl      company_website;

    QString   status; // "filed", "expected", "priced", "withdrawn"
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
};
