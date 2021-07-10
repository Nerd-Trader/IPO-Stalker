#pragma once

#include "data-source.hpp"

class DataSourceIpoCalAppSpot: public DataSource
{
public:
    explicit DataSourceIpoCalAppSpot(QObject *parent = nullptr);
    ~DataSourceIpoCalAppSpot();

    void queryData() final;

private:
    static QString translateSectorName(QString original);
};
