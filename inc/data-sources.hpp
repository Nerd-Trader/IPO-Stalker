#pragma once

#include <QTimer>

#include "data-sources/ipo-cal-appspot.hpp"
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

private:
    DataSourceIpoCalAppSpot *dataSourceJapan = new DataSourceIpoCalAppSpot();
    MainWindow *parentObject;
    QTimer *timer;
};
