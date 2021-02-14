#pragma once

#include <QDateTime>

enum Exchange {
    UNKNOWN = 0;   // Unknown
    NYSE    = 1;   // New York Stock Exchange
    NASDAQ  = 2;   // National Association of Securities Dealers Automated Quotations
    TSX     = 4;   // Toronto Stock Exchange
    SEHK    = 8;   // Hong Kong Exchange
    LSE     = 16;  // London Stock Exchange
    TSE     = 32;  // Tokyo Stock Exchange
    SZSE    = 64;  // Shenzhen Stock Exchange
    TADAWUL = 128; // Saudi Stock Exchange
    PINK    = 256; // OTC Markets Pink
    MEXI    = 512; // Mexican Stock Exchange
};

struct Ipo {
    QString   company_name;
    QDateTime expected_date;
    Exchange  stock_exchange,
    QString   ticker;
    QUrl      website;
};
