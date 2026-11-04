#pragma once

#include <QJsonArray>

#include "data-source.hpp"

class DataSourceNseIndia: public DataSource
{
public:
    explicit DataSourceNseIndia(QObject *parent = nullptr);
    ~DataSourceNseIndia();

    void queryData() final;

    static QDate parseDate(const QString& str);
    static QList<Ipo> parseIssues(const QByteArray& json, bool pastIssues);

private:
    void queryApi();
};
