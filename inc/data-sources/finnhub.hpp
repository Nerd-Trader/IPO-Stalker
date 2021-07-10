#pragma once

#include "data-source.hpp"

class DataSourceFinnhub : public DataSource
{
public:
    explicit DataSourceFinnhub(QObject *parent = nullptr);
    ~DataSourceFinnhub();

    void queryData() final;

private:
    QString getCurrentDate(int monthDiff);
};
