#pragma once

#include <QTimer>

#include "data-sources/ipo-cal-appspot.hpp"
#include "ipo.hpp"

class DataSources : public QObject
{
    Q_OBJECT

public:
    DataSources();

    QList<Ipo> getIpos();

public slots:
    void queryJapan();

private:
    DataSourceIpoCalAppSpot *dataSourceJapan = new DataSourceIpoCalAppSpot();
    QList<Ipo> ipos;
    QTimer *timer;
};
