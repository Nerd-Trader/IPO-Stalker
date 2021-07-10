#pragma once

#include "data-source.hpp"

class DataSourceNasdaq: public DataSource
{
public:
    explicit DataSourceNasdaq(QObject *parent = nullptr);
    ~DataSourceNasdaq();

    void queryData() final;
};
