#include <QDebug>
#include <QPointer>

#include "data-source.hpp"

#define DATA_SOURCE_REQUEST_TIMEOUT 30 // Seconds

DataSource::DataSource(QObject* parent) : QObject(parent)
{
    timer = new QTimer(this);

    // Periodic runs
    connect(timer, &QTimer::timeout, this, &DataSource::queryDataSlot);
}

DataSource::~DataSource()
{
}

void DataSource::get(const QUrl& url, const ReplyHandler& handler, const Headers& headers)
{
    if (networkAccessManager == nullptr) {
        networkAccessManager = new QNetworkAccessManager(this);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, DATA_SOURCE_USER_AGENT);
    request.setRawHeader("Accept-Language", "en-US,en;q=0.7");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(DATA_SOURCE_REQUEST_TIMEOUT * 1000);
#endif
    for (const QPair<QByteArray, QByteArray>& header : headers) {
        request.setRawHeader(header.first, header.second);
    }

    QNetworkReply* reply = networkAccessManager->get(request);
    QPointer<DataSource> self(this);

    connect(reply, &QNetworkReply::finished, this, [self, reply, handler]() {
        reply->deleteLater();

        if (self.isNull()) {
            return;
        }

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();

#ifdef DEBUG
        if (reply->error() != QNetworkReply::NoError) {
            qDebug().noquote() << QString("[%1] %2: %3").arg(*self->getName(), reply->url().toString(), reply->errorString());
        }
#endif

        handler(status, body);
    });
}

const QString* DataSource::getName()
{
    return &name;
}

void DataSource::queryDataSlot()
{
    lastUsed = QDateTime::currentDateTime();

    queryData();
}

void DataSource::setName(const QString name)
{
    // Make sure the name only gets set once
    if (this->name == DATA_SOURCE_DEFAULT_NAME) {
        this->name = name;
    }
}

void DataSource::setNetworkAccessManager(QNetworkAccessManager* manager)
{
    networkAccessManager = manager;
}

void DataSource::setQueryInterval(const int seconds)
{
    queryInterval = seconds;
}

void DataSource::start()
{
    // Initial run
    queryDataSlot();

    // Consequent periodic runs
    if (queryInterval > 0) {
        timer->start(queryInterval * 1000);
    }
}
