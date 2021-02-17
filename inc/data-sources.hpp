#pragma once

#include <QTimer>

#include "data-sources/ipo-cal-appspot.hpp"
#include "ipo.hpp"
#include "mainwindow.hpp"

class DataSources : public QObject
{
    Q_OBJECT

public:
    explicit DataSources(QObject *parent = nullptr);
    ~DataSources();

public slots:
    void callParentSlot();
    void queryJapan();

private:
    DataSourceIpoCalAppSpot *dataSourceJapan = new DataSourceIpoCalAppSpot();
    QTimer *timer;
};
