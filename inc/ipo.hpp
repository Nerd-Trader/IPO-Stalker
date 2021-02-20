#pragma once

#include <QDateTime>
#include <QUrl>

struct Ipo {
    QString   company_name;
    QUrl      company_website;
    QDateTime expected_date;
    QString   region;
    QString   status; // "expected", "priced", "withdrawn", "filed"
    QString   stock_exchange;
    QString   ticker;
};
