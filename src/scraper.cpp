#include <QDebug>
#include <QTimer>

#include "scraper.hpp"
#include "mainwindow.hpp"

Scraper::Scraper(QObject *parent) : QObject(parent)
{
    parentObject = (MainWindow *)this->parent();

    dataSources << new DataSourceIpoCalAppSpot(this);
    dataSources << new DataSourceFinnhub(this);
    dataSources << new DataSourceNasdaq(this);

    start();
}

Scraper::~Scraper()
{
    QVectorIterator<DataSource *> itDataSources(dataSources);

    while (itDataSources.hasNext()) {
        DataSource *dataSource = itDataSources.next();

        if (dataSource->isRunning()) {
            dataSource->quit();
            dataSource->wait();
        }
    }
}

void Scraper::processRetrievedData(DataSource *dataSource)
{
    QList<Ipo> *retrievedIpos = dataSource->retrievedIpos;
    int count = retrievedIpos->count();

    const QString dataSourceName = dataSource->getName();

    qDebug().noquote() << "Retrieved" << count << "records from" << dataSourceName;

    if (count > 0) {
        if (parentObject->db->processNewlyObtainedData(retrievedIpos, &dataSourceName)) {
            parentObject->updateList();
        }
    }
}

void Scraper::start()
{
    int i = 0;
    QVectorIterator<DataSource *> itDataSources(dataSources);
    int timePeriod = 10; // Seconds
    int timePeriodChunk = timePeriod / dataSources.size();

    while (itDataSources.hasNext()) {
        DataSource *dataSource = itDataSources.next();

        if (!dataSource->isRunning()) {
            QTimer::singleShot(++i * timePeriodChunk * 1000, [dataSource]() {
                dataSource->start();
            });
        }
    }
}
