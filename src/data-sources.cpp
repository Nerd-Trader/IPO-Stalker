#include "data-sources.hpp"
#include "mainwindow.hpp"

#define SECOND_IN_MS 1000
#define MINUTE_IN_MS 60 * SECOND_IN_MS
#define HOUR_IN_MS   60 * MINUTE_IN_MS

DataSources::DataSources(QObject *parent) : QObject(parent)
{
    parentObject = (MainWindow *)this->parent();

    // Start-time requests
    queryJapaneseIpos();

    // Consequitive requests
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(queryJapaneseIpos()));
    timer->start(8 * HOUR_IN_MS);
}

DataSources::~DataSources()
{
}

void DataSources::queryJapaneseIpos()
{
    QList<Ipo> retrievedIpos = dataSourceJapan->query();

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
