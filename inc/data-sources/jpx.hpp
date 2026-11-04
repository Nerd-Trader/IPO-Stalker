#pragma once

#include "data-source.hpp"

class DataSourceJpx: public DataSource
{
public:
    explicit DataSourceJpx(QObject *parent = nullptr);
    ~DataSourceJpx();

    void queryData() final;

    static QList<QDate> parseDates(const QString& str);
    static QList<Ipo> parsePage(const QString& html, const QDate& today);
};
