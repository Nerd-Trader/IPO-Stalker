#pragma once

#include "data-source.hpp"
#include "settings.hpp"

class DataSourceFinnhub : public DataSource
{
public:
    explicit DataSourceFinnhub(QObject *parent = nullptr);
    ~DataSourceFinnhub();

    void queryData() final;

private:
    QString getCurrentDate(int monthDiff);

    Settings *settings;
};
