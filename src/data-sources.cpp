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

QList<Ipo> DataSources::getIpos()
{
    return ipos;
}

void DataSources::queryJapan()
{
    QList<Ipo> retrieved_ipos = dataSourceJapan->query();

    // TODO:cherry-pick which items to remove, update, and insert
    ipos = retrieved_ipos;
}
