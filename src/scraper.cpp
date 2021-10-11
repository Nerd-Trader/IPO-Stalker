#include <QDebug>
#include <QThread>
#include <QTimer>

#include "scraper.hpp"
#include "mainwindow.hpp"

Scraper::Scraper(Db* db) : QThread()
{
    this->db = db;

    dataSources << new DataSourceEuronext(this);
    dataSources << new DataSourceFinnhub(this);
    dataSources << new DataSourceIpoCalAppSpot(this);
    dataSources << new DataSourceNasdaq(this);
    dataSources << new DataSourceOtcbbSwingtradebot(this);

    connect(this, SIGNAL(started()), this, SLOT(startSlot()));
}

Scraper::~Scraper()
{
    QVectorIterator<DataSource*> itDataSources(dataSources);

    while (itDataSources.hasNext()) {
        DataSource* dataSource = itDataSources.next();

        if (dataSource->isRunning()) {
            dataSource->quit();
            dataSource->wait();
        }
    }
}

void Scraper::processRetrievedIpoDataSlot(const QList<Ipo>* ipos, const QString dataSourceName)
{
#ifdef DEBUG
    qDebug().noquote() << QString("Retrieved IPO data for “%1” from [%2]").arg(ipo->company_name, dataSourceName);
#endif

    db->processNewlyObtainedData(ipos, &dataSourceName);
}

void Scraper::startSlot()
{
    int i = 0;
    QVectorIterator<DataSource*> itDataSources(dataSources);
    const int timePeriod = 10; // Seconds
    const int timePeriodChunk = timePeriod / dataSources.size();

    while (itDataSources.hasNext()) {
        DataSource *dataSource = itDataSources.next();

        connect(dataSource, SIGNAL(ipoInfoObtained(const QList<Ipo>*, const QString)), this, SLOT(processRetrievedIpoDataSlot(const QList<Ipo>*, const QString)));

        if (!dataSource->isRunning()) {
            QTimer::singleShot(++i * timePeriodChunk * 1000, [dataSource]() {
                dataSource->start();
            });
        }
    }
}
