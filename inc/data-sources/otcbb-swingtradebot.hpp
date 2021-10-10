#pragma once

#include <QWebEnginePage>

#include "data-source.hpp"

class DataSourceOtcbbSwingtradebot: public DataSource
{
public:
    explicit DataSourceOtcbbSwingtradebot(QObject *parent = nullptr);
    ~DataSourceOtcbbSwingtradebot();

    void parseMainPage(QWebEnginePage *page);
    void queryData() final;
};
