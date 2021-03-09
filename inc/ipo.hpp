#pragma once

#include <QDateTime>
#include <QUrl>

struct Ipo {
    QString   company_name;
    QUrl      company_website;
    QDateTime expected_date;
    QString   region;
    QString   market_sector;
    QString   status; // "filed", "expected", "priced", "withdrawn"
    QString   stock_exchange;
    QString   ticker;
};

/*
//                    priced
//                  /
// filed → expected
//      \___________\
//                    withdrawn
*/
