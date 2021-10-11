#pragma once

#include <QWebEnginePage>

#include "data-source.hpp"

class DataSourceEdelweiss: public DataSource
{
public:
    explicit DataSourceEdelweiss(QObject *parent = nullptr);
    ~DataSourceEdelweiss();

    void queryData() final;
};
