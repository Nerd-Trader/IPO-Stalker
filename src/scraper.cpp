#include <QDebug>

#include "scraper.hpp"
#include "mainwindow.hpp"

Scraper::Scraper(QObject *parent) : QObject(parent)
{
    parentObject = (MainWindow *)this->parent();

    dataSources << new DataSourceIpoCalAppSpot(this);
    dataSources << new DataSourceFinnhub(this);
    dataSources << new DataSourceNasdaq(this);

    // Perform initial data query from all data sources
    QVectorIterator<DataSource *> itDataSources(dataSources);
    while (itDataSources.hasNext()) {
        itDataSources.next()->queryDataSlot();
    }
}

Scraper::~Scraper()
{
}

void Scraper::processQueriedData(DataSource *dataSource)
{
    QList<Ipo> retrievedIpos = *dataSource->retrievedIpos;
    int count = retrievedIpos.count();

    qDebug() << "Retrieved" << count << "IPOs from" << dataSource->getName();

    if (count > 0) {
        QList<Ipo>::const_iterator retrievedIpo;
        for (retrievedIpo = retrievedIpos.constBegin(); retrievedIpo != retrievedIpos.constEnd(); ++retrievedIpo) {
            bool exists = false;

            for (QList<Ipo>::iterator i = parentObject->ipos.begin(); i != parentObject->ipos.end(); ++i) {
                if (i->company_name == (*retrievedIpo).company_name) {
                    if ((*retrievedIpo).filed_date > i->filed_date) {
                        i->filed_date = (*retrievedIpo).filed_date;
                        if (i->status == IPO_STATUS_UNKNOWN) {
                            i->status = IPO_STATUS_FILED;
                        }
                    }
                    if ((*retrievedIpo).expected_date > i->expected_date) {
                        i->expected_date = (*retrievedIpo).expected_date;
                        if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED) {
                            i->status = IPO_STATUS_EXPECTED;
                        }
                    }
                    if ((*retrievedIpo).priced_date > i->priced_date) {
                        i->priced_date = (*retrievedIpo).priced_date;
                        if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED || i->status == IPO_STATUS_EXPECTED) {
                            i->status = IPO_STATUS_PRICED;
                        }
                    }
                    if ((*retrievedIpo).withdrawn_date > i->withdrawn_date) {
                        i->withdrawn_date = (*retrievedIpo).withdrawn_date;
                        if (i->status != IPO_STATUS_WITHDRAWN) {
                            i->status = IPO_STATUS_WITHDRAWN;
                        }
                    }
                    if ((*retrievedIpo).market_sector != "") {
                        i->market_sector = (*retrievedIpo).market_sector;
                    }
                    if ((*retrievedIpo).company_website.toString() != "") {
                        i->company_website = (*retrievedIpo).company_website;
                    }

                    if (!i->sources.contains(dataSource->getName())) {
                        i->sources << dataSource->getName();
                    }

                    exists = true;

                    break;
                }
            }

            if (!exists) {
                parentObject->ipos.append(*retrievedIpo);
                parentObject->ipos.last().sources << dataSource->getName();
            }
        }

        parentObject->updateList();
    }
}
