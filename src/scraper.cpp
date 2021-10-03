#include <QDebug>
#include <QTimer>

#include "scraper.hpp"
#include "mainwindow.hpp"

Scraper::Scraper(QObject *parent) : QObject(parent)
{
    parentObject = (MainWindow *)this->parent();

    dataSources << new DataSourceEuronext(this);
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

void Scraper::processRetrievedIpoData(const Ipo *ipo, const QString dataSourceName)
{
    qDebug().noquote() << QString("Retrieved IPO data for “%1” from [%2]").arg(ipo->company_name, dataSourceName);

    if (parentObject->db->processNewlyObtainedData(ipo, &dataSourceName)) {
        parentObject->updateList();
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

        connect(dataSource, SIGNAL(ipoInfoObtained(const Ipo*, const QString)), this, SLOT(processRetrievedIpoData(const Ipo*, const QString)));

        if (!dataSource->isRunning()) {
            QTimer::singleShot(++i * timePeriodChunk * 1000, [dataSource]() {
                dataSource->start();
            });
        }
    }
}
