#pragma once

#include <QDateTime>

struct Ipo {
    QString   company_name;
    QDateTime expected_date;
    QString   stock_exchange,
    QString   ticker;
    QUrl      website;
};
