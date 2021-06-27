#include "data-sources.hpp"
#include "mainwindow.hpp"

#define SECOND_IN_MS 1000
#define MINUTE_IN_MS 60 * SECOND_IN_MS
#define HOUR_IN_MS   60 * MINUTE_IN_MS

DataSources::DataSources(QObject *parent) : QObject(parent)
{
    parentObject = (MainWindow *)this->parent();

    dataSourceJapanIpos = new DataSourceIpoCalAppSpot(this);
    QString finnhubApiKey;
    if (parentObject->settings->contains("Secrets/finnhubApiKey")) {
        finnhubApiKey = parentObject->settings->value("Secrets/finnhubApiKey").toString();
    }
    dataSourceUsIpos = new DataSourceFinnhub(this, finnhubApiKey);
    dataSourceUsIpos2 = new DataSourceNasdaq(this);

    // Start-time requests
    queryJapaneseIpos();
    queryUsIpos();
    queryUsIpos2();

    // Recurring requests
    // Data source: ipo-cal.appspot.com
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(queryJapaneseIpos()));
    timer->start(8 * HOUR_IN_MS);
    // Data source: finnhub.io
    timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(queryUsIpos()));
    timer2->start(2 * HOUR_IN_MS);
    // Data source: nasdaq.com
    timer3 = new QTimer(this);
    connect(timer3, SIGNAL(timeout()), this, SLOT(queryUsIpos2()));
    timer3->start(6 * HOUR_IN_MS);
}

DataSources::~DataSources()
{
}

void DataSources::queryJapaneseIpos()
{
    QList<Ipo> retrievedIpos = dataSourceJapanIpos->queryData();
    int count = retrievedIpos.count();

    qDebug() << "Retrieved" << count << "IPOs from" << DATA_SOURCE_IPO_CAL_APPSPOT_SOURCE_NAME;

    if (count > 0) {
        foreach (Ipo retrievedIpo, retrievedIpos) {
            bool exists = false;

            for (QList<Ipo>::iterator i = parentObject->ipos.begin(); i != parentObject->ipos.end(); ++i) {
                if (i->company_name == retrievedIpo.company_name) {
                    if (retrievedIpo.filed_date > i->filed_date) {
                        i->filed_date = retrievedIpo.filed_date;
                        if (i->status == IPO_STATUS_UNKNOWN) {
                            i->status = IPO_STATUS_FILED;
                        }
                    }
                    if (retrievedIpo.expected_date > i->expected_date) {
                        i->expected_date = retrievedIpo.expected_date;
                        if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED) {
                            i->status = IPO_STATUS_EXPECTED;
                        }
                    }
                    if (retrievedIpo.priced_date > i->priced_date) {
                        i->priced_date = retrievedIpo.priced_date;
                        if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED || i->status == IPO_STATUS_EXPECTED) {
                            i->status = IPO_STATUS_PRICED;
                        }
                    }
                    if (retrievedIpo.withdrawn_date > i->withdrawn_date) {
                        i->withdrawn_date = retrievedIpo.withdrawn_date;
                        if (i->status != IPO_STATUS_WITHDRAWN) {
                            i->status = IPO_STATUS_WITHDRAWN;
                        }
                    }
                    if (retrievedIpo.market_sector != "") {
                        i->market_sector = retrievedIpo.market_sector;
                    }
                    if (retrievedIpo.company_website.toString() != "") {
                        i->company_website = retrievedIpo.company_website;
                    }

                    for (QStringList::const_iterator j = retrievedIpo.sources.constBegin(); j != retrievedIpo.sources.constEnd(); ++j) {
                        if (!i->sources.contains(*j)) {
                            i->sources << *j;
                        }
                    }

                    exists = true;

                    break;
                }
            }

            if (!exists) {
                parentObject->ipos.append(retrievedIpo);
            }
        }

        parentObject->updateList();
    }
}

void DataSources::queryUsIpos()
{
    QList<Ipo> retrievedIpos = dataSourceUsIpos->queryData();
    int count = retrievedIpos.count();

    qDebug() << "Retrieved" << count << "IPOs from" << DATA_SOURCE_FINNHUB_SOURCE_NAME;

    if (count > 0) {
        foreach (Ipo retrievedIpo, retrievedIpos) {
            bool exists = false;

            for (QList<Ipo>::iterator i = parentObject->ipos.begin(); i != parentObject->ipos.end(); ++i) {
                if (i->company_name == retrievedIpo.company_name) {
                    if (retrievedIpo.filed_date > i->filed_date) {
                        i->filed_date = retrievedIpo.filed_date;
                        if (i->status == IPO_STATUS_UNKNOWN) {
                            i->status = IPO_STATUS_FILED;
                        }
                    }
                    if (retrievedIpo.expected_date > i->expected_date) {
                        i->expected_date = retrievedIpo.expected_date;
                        if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED) {
                            i->status = IPO_STATUS_EXPECTED;
                        }
                    }
                    if (retrievedIpo.priced_date > i->priced_date) {
                        i->priced_date = retrievedIpo.priced_date;
                        if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED || i->status == IPO_STATUS_EXPECTED) {
                            i->status = IPO_STATUS_PRICED;
                        }
                    }
                    if (retrievedIpo.withdrawn_date > i->withdrawn_date) {
                        i->withdrawn_date = retrievedIpo.withdrawn_date;
                        if (i->status != IPO_STATUS_WITHDRAWN) {
                            i->status = IPO_STATUS_WITHDRAWN;
                        }
                    }
                    if (retrievedIpo.market_sector != "") {
                        i->market_sector = retrievedIpo.market_sector;
                    }
                    if (retrievedIpo.company_website.toString() != "") {
                        i->company_website = retrievedIpo.company_website;
                    }

                    for (QStringList::const_iterator j = retrievedIpo.sources.constBegin(); j != retrievedIpo.sources.constEnd(); ++j) {
                        if (!i->sources.contains(*j)) {
                            i->sources << *j;
                        }
                    }

                    exists = true;

                    break;
                }
            }

            if (!exists) {
                parentObject->ipos.append(retrievedIpo);
            }
        }

        parentObject->updateList();
    }
}

void DataSources::queryUsIpos2()
{
    QList<Ipo> retrievedIpos = dataSourceUsIpos2->queryData();
    int count = retrievedIpos.count();

    qDebug() << "Retrieved" << count << "IPOs from" << DATA_SOURCE_NASDAQ_SOURCE_NAME;

    if (count > 0) {
        foreach (Ipo retrievedIpo, retrievedIpos) {
            bool exists = false;

            for (QList<Ipo>::iterator i = parentObject->ipos.begin(); i != parentObject->ipos.end(); ++i) {
                if (i->company_name == retrievedIpo.company_name) {
                    if (retrievedIpo.filed_date > i->filed_date) {
                        i->filed_date = retrievedIpo.filed_date;
                        if (i->status == IPO_STATUS_UNKNOWN) {
                            i->status = IPO_STATUS_FILED;
                        }
                    }
                    if (retrievedIpo.expected_date > i->expected_date) {
                        i->expected_date = retrievedIpo.expected_date;
                        if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED) {
                            i->status = IPO_STATUS_EXPECTED;
                        }
                    }
                    if (retrievedIpo.priced_date > i->priced_date) {
                        i->priced_date = retrievedIpo.priced_date;
                        if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED || i->status == IPO_STATUS_EXPECTED) {
                            i->status = IPO_STATUS_PRICED;
                        }
                    }
                    if (retrievedIpo.withdrawn_date > i->withdrawn_date) {
                        i->withdrawn_date = retrievedIpo.withdrawn_date;
                        if (i->status != IPO_STATUS_WITHDRAWN) {
                            i->status = IPO_STATUS_WITHDRAWN;
                        }
                    }
                    if (retrievedIpo.market_sector != "") {
                        i->market_sector = retrievedIpo.market_sector;
                    }
                    if (retrievedIpo.company_website.toString() != "") {
                        i->company_website = retrievedIpo.company_website;
                    }

                    for (QStringList::const_iterator j = retrievedIpo.sources.constBegin(); j != retrievedIpo.sources.constEnd(); ++j) {
                        if (!i->sources.contains(*j)) {
                            i->sources << *j;
                        }
                    }

                    exists = true;

                    break;
                }
            }

            if (!exists) {
                parentObject->ipos.append(retrievedIpo);
            }
        }

        parentObject->updateList();
    }
}
