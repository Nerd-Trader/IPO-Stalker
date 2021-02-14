#pragma once

#include <QDateTime>

enum Exchange {
    UNKNOWN = 0;  // Unknown
    NYSE    = 1;  // New York Stock Exchange
    NASDAQ  = 2;  // National Association of Securities Dealers Automated Quotations
    TSX     = 4;  // Toronto Stock Exchange
    HKEX    = 8;  // Hong Kong Exchanges and Clearing Limited
    LSE     = 16; // London Stock Exchange
};

struct Ipo {
    QString   company_name;
    QDateTime expected_date;
    Exchange  stock_exchange,
    QString   ticker;
};
