#pragma once

#include <QDateTime>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "ipo.hpp"

#define DATA_SOURCE_DEFAULT_NAME "UNNAMED"

class DataSource: public QThread
{
    Q_OBJECT

public:
    explicit DataSource(QObject* parent = nullptr);
    ~DataSource();

    const QString* getName();
    void forceQueryData();
    void setName(const QString name);
    void setQueryInterval(const int seconds);

    virtual void queryData() {};

public slots:
    void queryDataSlot();

signals:
    void ipoInfoObtainedSignal(const QList<Ipo>* ipos);

private:
    QDateTime lastUsed;
    QString name = DATA_SOURCE_DEFAULT_NAME;
    QTimer* timer;
};
