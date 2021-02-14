#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

class IpoCalAppSpot: public QObject
{
    Q_OBJECT

public:
    explicit IpoCalAppSpot(QObject *parent = 0);
    ~IpoCalAppSpot();

    void query();

private slots:
    void ready();

private:
    QString baseUrl = QString("https://ipo-cal.appspot.com/api");
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    QUrl url = QUrl(baseUrl + "/ipo");
};
