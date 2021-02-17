#include "data-sources.hpp"
#include "mainwindow.hpp"

DataSources::DataSources(QObject *parent) : QObject(parent)
{
    // Start-time requests
    queryJapan();

    // Consequitive requests
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(queryJapan()));
    timer->start(1000 * 60 * 60 * 2);
}

DataSources::~DataSources()
{
}

void DataSources::callParentSlot()
{
    ((MainWindow *)this->parent())->updateList();
}

void DataSources::queryJapan()
{
    QList<Ipo> retrieved_ipos = dataSourceJapan->query();

    // TODO: cherry-pick which items to remove, update, and insert
    ((MainWindow *)this->parent())->ipos = retrieved_ipos;

    callParentSlot();
}
