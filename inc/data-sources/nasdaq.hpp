#pragma once

#include <QJsonArray>

#include "data-source.hpp"

class DataSourceNasdaq: public DataSource
{
public:
    explicit DataSourceNasdaq(QObject *parent = nullptr);
    ~DataSourceNasdaq();

    void queryData() final;

    static QList<Ipo> parseCalendar(const QByteArray& json);

private:
    static void parseRows(const QJsonArray& rows, const IpoStatus status, QList<Ipo>* ipos);
    void parseReply(int status, const QByteArray& body);
    void scrapeMonthsData(const QDate& targetDate);
};
