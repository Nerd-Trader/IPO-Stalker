#pragma once

#include <QTimer>

#include "data-sources/finnhub.hpp"
#include "data-sources/ipo-cal-appspot.hpp"
#include "data-sources/nasdaq.hpp"
#include "ipo.hpp"
#include "mainwindow.hpp"

class MainWindow; // Forward declaration

class DataSources : public QObject
{
    Q_OBJECT

public:
    explicit DataSources(QObject *parent = nullptr);
    ~DataSources();

public slots:
    void queryJapaneseIpos();
    void queryUsIpos();
    void queryUsIpos2();

private:
    DataSourceIpoCalAppSpot *dataSourceJapanIpos;
    DataSourceFinnhub *dataSourceUsIpos;
    DataSourceNasdaq *dataSourceUsIpos2;
    MainWindow *parentObject;
    QTimer *timer;
    QTimer *timer2;
    QTimer *timer3;
};
