#pragma once

#include "data-source.hpp"

class DataSourceEuronext: public DataSource
{
public:
    explicit DataSourceEuronext(QObject *parent = nullptr);
    ~DataSourceEuronext();

    void queryData() final;

    static QList<Ipo> parsePage(const QString& html, const QDate& today);

private:
    static QList<IpoRegion> tradingLocationsToIpoRegions(const QString& tradingLocations);
};
