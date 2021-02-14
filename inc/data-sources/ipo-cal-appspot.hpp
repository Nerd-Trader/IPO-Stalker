#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

class DataSourceIpoCalAppSpot : public QObject
{
    Q_OBJECT

public:
    explicit DataSourceIpoCalAppSpot(QObject *parent = 0);
    ~DataSourceIpoCalAppSpot();

    void query();

private:
    QString baseUrl = QString("https://ipo-cal.appspot.com/api");
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    QUrl url = QUrl(baseUrl + "/ipo");
};
