#pragma once

#include <QWebEnginePage>

#include "data-source.hpp"

class DataSourceEuronext: public DataSource
{
public:
    explicit DataSourceEuronext(QObject *parent = nullptr);
    ~DataSourceEuronext();

    void parseMainPage(QWebEnginePage *page);
    void queryData() final;

private:
    QList<IpoRegion> tradingLocationsToIpoRegions(const QString tradingLocations);
};
