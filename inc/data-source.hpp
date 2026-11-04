#pragma once

#include <functional>

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QUrl>

#include "ipo.hpp"

#define DATA_SOURCE_DEFAULT_NAME "UNNAMED"
#define DATA_SOURCE_USER_AGENT   "Mozilla/5.0 (X11; Linux x86_64; rv:140.0) Gecko/20100101 Firefox/140.0"

// Base class for all data sources.
// All network I/O is asynchronous and happens on the main thread's event loop;
// data sources never block and never spin their own event loops.
class DataSource : public QObject
{
    Q_OBJECT

public:
    typedef QList<QPair<QByteArray, QByteArray>> Headers;
    typedef std::function<void(int httpStatus, const QByteArray& body)> ReplyHandler;

    explicit DataSource(QObject* parent = nullptr);
    ~DataSource();

    const QString* getName();
    void setNetworkAccessManager(QNetworkAccessManager* manager);
    void start();

    virtual void queryData() {};

public slots:
    void queryDataSlot();

signals:
    void ipoInfoObtainedSignal(const QList<Ipo>& ipos);

protected:
    void get(const QUrl& url, const ReplyHandler& handler, const Headers& headers = Headers());
    void setName(const QString name);
    void setQueryInterval(const int seconds);

    QNetworkAccessManager* networkAccessManager = nullptr;

private:
    QDateTime lastUsed;
    QString name = DATA_SOURCE_DEFAULT_NAME;
    int queryInterval = 0;
    QTimer* timer;
};
