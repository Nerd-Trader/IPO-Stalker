#include "data-source.hpp"
#include "data-sources.hpp"

DataSource::DataSource(QObject *parent) : QObject(parent)
{
    name = "UNNAMED";
    timer = new QTimer(this);
    retrievedIpos = new QList<Ipo>;

    connect(timer, SIGNAL(timeout()), this, SLOT(queryDataSlot()));
}

DataSource::~DataSource()
{
}

QString DataSource::getName()
{
    return name;
}

void DataSource::queryDataSlot()
{
    preQueryData();
    queryData();
    postQueryData();
}

void DataSource::preQueryData()
{
    lastUsed = QDateTime::currentDateTime();

    // Empty array of retrieved IPOs prior each run
    retrievedIpos->clear();
}

void DataSource::postQueryData()
{
    ((DataSources *)parent())->processQueriedData(this);
}

void DataSource::setName(QString name)
{
    this->name = name;
}

void DataSource::setQueryInterval(int seconds)
{
    timer->start(seconds * 1000);
}
