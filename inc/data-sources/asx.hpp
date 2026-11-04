#pragma once

#include "data-source.hpp"

class DataSourceAsx: public DataSource
{
public:
    explicit DataSourceAsx(QObject *parent = nullptr);
    ~DataSourceAsx();

    void queryData() final;

    static QDate parseDate(const QString& str);
    static QList<Ipo> parsePage(const QString& html);
};
