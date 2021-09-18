#include "data-source.hpp"
#include "scraper.hpp"

DataSource::DataSource(QObject *parent) : QThread(parent)
{
    name = "UNNAMED";
    timer = new QTimer(this);
    retrievedIpos = new QList<Ipo>;

    // Initial run
    connect(this, SIGNAL(started()), this, SLOT(queryDataSlot()));

    // Periodic consequental runs
    connect(timer, SIGNAL(timeout()), this, SLOT(queryDataSlot()));
}

DataSource::~DataSource()
{
    delete timer;
}

QString DataSource::getName()
{
    return name;
}

void DataSource::queryDataSlot()
{
    preQueryData();
    queryData();
    if (retrievedIpos->size() > 0) {
        postQueryData();
    }
}

void DataSource::preQueryData()
{
    lastUsed = QDateTime::currentDateTime();
}

void DataSource::postQueryData()
{
    ((Scraper *)parent())->processQueriedData(this);

    // Empty array of this data source's IPOs after each run
    retrievedIpos->clear();
}

void DataSource::setName(QString name)
{
    this->name = name;
}

void DataSource::setQueryInterval(int seconds)
{
    timer->start(seconds * 1000);
}
