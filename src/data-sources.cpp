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
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(queryJapaneseIpos()));
    timer->start(8 * HOUR_IN_MS);
    //
    timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(queryUsIpos()));
    timer->start(2 * HOUR_IN_MS);
    //
    timer3 = new QTimer(this);
    connect(timer3, SIGNAL(timeout()), this, SLOT(queryUsIpos2()));
    timer->start(6 * HOUR_IN_MS);
}

DataSources::~DataSources()
{
}

void DataSources::queryJapaneseIpos()
{
    QList<Ipo> retrievedIpos = dataSourceJapanIpos->queryData();

    foreach (Ipo retrievedIpo, retrievedIpos) {
        Ipo *existingIpo = nullptr;

        foreach (Ipo ipo, parentObject->ipos) {
            if (ipo.company_name == retrievedIpo.company_name) {
                existingIpo = &ipo;
            }
        }

        if (existingIpo) {
            existingIpo = &retrievedIpo;
        } else {
            parentObject->ipos.append(retrievedIpo);
        }
    }

    parentObject->updateList();
}

void DataSources::queryUsIpos()
{
    QList<Ipo> retrievedIpos = dataSourceUsIpos->queryData();

    foreach (Ipo retrievedIpo, retrievedIpos) {
        Ipo *existingIpo = nullptr;

        foreach (Ipo ipo, parentObject->ipos) {
            if (ipo.company_name == retrievedIpo.company_name) {
                existingIpo = &ipo;
            }
        }

        if (existingIpo) {
            existingIpo = &retrievedIpo;
        } else {
            parentObject->ipos.append(retrievedIpo);
        }
    }

    parentObject->updateList();
}

void DataSources::queryUsIpos2()
{
    QList<Ipo> retrievedIpos = dataSourceUsIpos2->queryData();

    foreach (Ipo retrievedIpo, retrievedIpos) {
        Ipo *existingIpo = nullptr;

        foreach (Ipo ipo, parentObject->ipos) {
            if (ipo.company_name == retrievedIpo.company_name) {
                existingIpo = &ipo;
            }
        }

        if (existingIpo) {
            existingIpo = &retrievedIpo;
        } else {
            parentObject->ipos.append(retrievedIpo);
        }
    }

    parentObject->updateList();
}
