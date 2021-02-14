#include "data-sources.hpp"

DataSources::DataSources()
{
    // Start-time requests
    queryJapan();

    // Consequitive requests
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(queryJapan()));
    timer->start(1000 * 60 * 60 * 2);
}

void DataSources::queryJapan()
{
    dataSourceJapan->query();
}
