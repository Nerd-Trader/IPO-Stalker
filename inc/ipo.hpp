#pragma once

#include <QDateTime>

struct Ipo {
    QString   company_name;
    QUrl      company_website;
    QDateTime expected_date;
    QString   stock_exchange,
    QString   ticker;
};
