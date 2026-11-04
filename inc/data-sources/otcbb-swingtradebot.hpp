#pragma once

#include "data-source.hpp"

class DataSourceOtcbbSwingtradebot: public DataSource
{
public:
    explicit DataSourceOtcbbSwingtradebot(QObject *parent = nullptr);
    ~DataSourceOtcbbSwingtradebot();

    void queryData() final;

    static QList<Ipo> parsePage(const QString& html, const QDate& today);
};
