#include "data-source.hpp"
#include "scraper.hpp"

DataSource::DataSource(QObject *parent) : QThread(parent)
{
    timer = new QTimer(this);

    // Initial run
    connect(this, SIGNAL(started()), this, SLOT(queryDataSlot()));

    // Consequental periodic runs
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
    lastUsed = QDateTime::currentDateTime();

    queryData();
}

void DataSource::setName(const QString name)
{
    // Make sure this doesn't get changed
    if (this->name == DATA_SOURCE_DEFAULT_NAME) {
        this->name = name;
    }
}

void DataSource::setQueryInterval(const int seconds)
{
    timer->start(seconds * 1000);
}
