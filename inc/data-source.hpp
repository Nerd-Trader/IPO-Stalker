#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>

#include "ipo.hpp"

class DataSource: public QObject
{
    Q_OBJECT

public:
    explicit DataSource(QObject *parent = nullptr);
    ~DataSource();

    QString getName();
    void forceQueryData();
    void setName(QString name);
    void setQueryInterval(int seconds);

    QList<Ipo> *retrievedIpos = nullptr;

    virtual void queryData() {};

public slots:
    void queryDataSlot();

private:
    QDateTime lastUsed;
    QString name;
    QTimer *timer;

    void preQueryData();
    void postQueryData();
};
