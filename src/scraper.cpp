#include <QDebug>
#include <QTimer>

#include "data-sources/asx.hpp"
#include "data-sources/euronext.hpp"
#include "data-sources/finnhub.hpp"
#include "data-sources/jpx.hpp"
#include "data-sources/nasdaq.hpp"
#include "data-sources/nseindia.hpp"
#include "data-sources/otcbb-swingtradebot.hpp"
#include "scraper.hpp"

#define SCRAPER_INITIAL_RUN_TIME_FRAME 30 // Seconds

Scraper::Scraper(Db* db, QObject* parent) : QObject(parent), db(db)
{
    networkAccessManager = new QNetworkAccessManager(this);

    dataSources << new DataSourceAsx(this);
    dataSources << new DataSourceEuronext(this);
    dataSources << new DataSourceFinnhub(this);
    dataSources << new DataSourceJpx(this);
    dataSources << new DataSourceNasdaq(this);
    dataSources << new DataSourceNseIndia(this);
    dataSources << new DataSourceOtcbbSwingtradebot(this);

    for (DataSource* dataSource : dataSources) {
        dataSource->setNetworkAccessManager(networkAccessManager);

        connect(dataSource, &DataSource::ipoInfoObtainedSignal, this, [this, dataSource](const QList<Ipo>& ipos) {
#ifdef DEBUG
            qDebug().noquote() << QString("Retrieved %1 IPO record(s) from [%2]").arg(ipos.size()).arg(*dataSource->getName());
#endif

            this->db->processNewlyObtainedData(&ipos, dataSource->getName());
        });
    }
}

Scraper::~Scraper()
{
}

void Scraper::start()
{
    // Spread initial queries across a time frame to avoid a burst of network activity on startup
    const int timePeriod = SCRAPER_INITIAL_RUN_TIME_FRAME * 1000 / qMax(1, (int)dataSources.size());

    int i = 0;
    for (DataSource* dataSource : dataSources) {
        QTimer::singleShot(i++ * timePeriod, dataSource, &DataSource::start);
    }
}
