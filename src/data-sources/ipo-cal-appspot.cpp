/*
 * IPO Calendar data source: Japanese IPOs
 * API Spec: https://ipo-cal.appspot.com/apispec.html
 *
 */

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QTextCodec>

#include "data-sources/ipo-cal-appspot.hpp"

DataSourceIpoCalAppSpot::DataSourceIpoCalAppSpot(QObject *parent) : QObject(parent)
{
}

DataSourceIpoCalAppSpot::~DataSourceIpoCalAppSpot()
{
    delete reply;
}

QString DataSourceIpoCalAppSpot::translateSectorName(QString original)
{
    if (original == "サービス") {
        return "Service";
    } else if (original == "機械") {
        return "Machine";
    } else if (original == "情報・通信") {
        return "Telecommunications";
    } else if (original == "小売") {
        return "Retail";
    } else if (original == "証券") {
        return "Securities";
    } else if (original == "化学") {
        return "Chemistry";
    } else if (original == "その他製品") {
        return "Other products";
    } else if (original == "その他金融") {
        return "Other finance";
    } else if (original == "電気機器") {
        return "Electrical equipment";
    } else if (original == "陸運") {
        return "Land transportation";
    } else if (original == "不動産") {
        return "Real estate";
    } else if (original == "医薬品") {
        return "Pharmaceuticals";
    } else if (original == "倉庫・運輸") {
        return "Warehouse / Transportation";
    } else if (original == "保険") {
        return "Insurance";
    } else if (original == "卸売") {
        return "Wholesale";
    } else if (original == "建設") {
        return "Construction";
    } else if (original == "食料品") {
        return "Grocery";
    } else {
        return original;
    }
}

QList<Ipo> DataSourceIpoCalAppSpot::queryData()
{
    QNetworkRequest request(url);
    QList<Ipo> retrievedIpos;

    lastUsed = QDateTime::currentDateTime();

    reply = manager.get(request);

    while (!reply->isFinished()) {
        QCoreApplication::processEvents();
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int status = statusCode.toInt();

    if (status == 200) {
        QJsonParseError jsonParseError;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &jsonParseError);

        if (jsonParseError.error != QJsonParseError::NoError) {
            return retrievedIpos;
        }

        QJsonObject jsonRoot = jsonDocument.object();

        if (jsonRoot["result"] == QJsonValue::Undefined) {
            return retrievedIpos;
        }

        QJsonArray dataArray = jsonRoot["data"].toArray();

        foreach (const QJsonValue &item, dataArray) {
            Ipo ipo;
            QJsonObject ipoObj = item.toObject();

            if (ipoObj["title"] == QJsonValue::Undefined) {
                continue;
            }

            ipo.company_name = ipoObj["name"].toString().replace("（株）", "");
            ipo.company_website = QUrl(ipoObj["url"].toString());
            ipo.expected_date = QDateTime::fromString(ipoObj["date"].toString(), "yyyy/MM/dd");
            QString sector = ipoObj["sector_name"].toString();
            if (sector.size() > 0 && sector != "-") {
                ipo.market_sector = translateSectorName(sector);
            }
            ipo.region = QString("🇯🇵 Asia (Japan)");
            ipo.stock_exchange = QString("TSE (%1)").arg(ipoObj["market_key"].toString());
            ipo.ticker = ipoObj["code"].toString();

            retrievedIpos.append(ipo);
        }
    }

    reply->deleteLater();

    return retrievedIpos;
}
