#include <QDebug>
#include <QThread>
#include <QTimer>

#include "data-sources/edelweiss.hpp"
#include "data-sources/euronext.hpp"
#include "data-sources/finnhub.hpp"
#include "data-sources/ipo-cal-appspot.hpp"
#include "data-sources/nasdaq.hpp"
#include "data-sources/otcbb-swingtradebot.hpp"
#include "mainwindow.hpp"
#include "scraper.hpp"

#define SCRAPER_INITIAL_RUN_TIME_FRAME 30 // Seconds

Scraper::Scraper(Db* db) : QThread()
{
    this->db = db;

    dataSources << new DataSourceEdelweiss(this);
    dataSources << new DataSourceEuronext(this);
    dataSources << new DataSourceFinnhub(this);
    dataSources << new DataSourceIpoCalAppSpot(this);
    dataSources << new DataSourceNasdaq(this);
    dataSources << new DataSourceOtcbbSwingtradebot(this);

    connect(this, SIGNAL(started()), this, SLOT(startSlot()));
}

Scraper::~Scraper()
{
    foreach (DataSource *dataSource, dataSources) {
        if (dataSource->isRunning()) {
            dataSource->quit();
            dataSource->wait();
        }
    }
}

void Scraper::startSlot()
{
    const int timeFrame = SCRAPER_INITIAL_RUN_TIME_FRAME;
    const int timePeriod = timeFrame / dataSources.size();

    int i = 0;
    foreach (DataSource *dataSource, dataSources) {
        const QString* dataSourceName = dataSource->getName();

        connect(dataSource, &DataSource::ipoInfoObtainedSignal, this, [this, dataSourceName](const QList<Ipo>* ipos){
#ifdef DEBUG
            qDebug().noquote() << QString("Retrieved IPO data for “%1” from [%2]").arg(ipo->company_name, dataSourceName);
#endif

            db->processNewlyObtainedData(ipos, dataSourceName);
        });

        if (!dataSource->isRunning()) {
            QTimer::singleShot(++i * timePeriod * 1000, [dataSource]() {
                dataSource->start();
            });
        }
    }
}
