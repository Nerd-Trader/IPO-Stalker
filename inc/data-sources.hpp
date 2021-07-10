#pragma once

#include <QTimer>

#include "data-source.hpp"
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

    QVector<DataSource *> dataSources;
    MainWindow *parentObject;

    void processQueriedData(DataSource *dataSource);
};
