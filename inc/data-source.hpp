#pragma once

#include <QDateTime>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "ipo.hpp"

class DataSource: public QThread
{
    Q_OBJECT

public:
    explicit DataSource(QObject *parent = nullptr);
    ~DataSource();

    QString getName();
    void forceQueryData();
    void setName(const QString name);
    void setQueryInterval(const int seconds);

    virtual void queryData() {};

public slots:
    void queryDataSlot();

signals:
    void ipoInfoObtained(const Ipo *ipo, const QString dataSourceName);

private:
    QDateTime lastUsed;
    QString name = "UNNAMED";
    QTimer *timer;
};
