/*
 * IPO Stalker data source: recent OTC IPOs
 *
 */

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrlQuery>
#include <QWebEngineScript>
#include <QWebEngineSettings>

#include "data-sources/otcbb-swingtradebot.hpp"
#include "ipo.hpp"

#define DATA_SOURCE_OTCBBSWINGTRADEBOT_SOURCE_NAME "otcbb.swingtradebot.com"

DataSourceOtcbbSwingtradebot::DataSourceOtcbbSwingtradebot(QObject *parent) : DataSource(parent)
{
    DataSource::setName(DATA_SOURCE_OTCBBSWINGTRADEBOT_SOURCE_NAME);
    DataSource::setQueryInterval(6 * 60 * 60);
}

DataSourceOtcbbSwingtradebot::~DataSourceOtcbbSwingtradebot()
{
}

void DataSourceOtcbbSwingtradebot::parseMainPage(QWebEnginePage *page)
{
    page->runJavaScript(" \
        const items = []; \
        const tbodyEl = document.getElementsByTagName('TBODY')[0]; \
        const rowEls = tbodyEl.getElementsByTagName('TR'); \
        for (let i = 0, ilen = rowEls.length; i < ilen; i++) { \
            const item = {}; \
            const colEls = rowEls[i].getElementsByTagName('TD'); \
            for (let j = 0, jlen = colEls.length; j < jlen; j++) { \
                if (j == 1) { \
                    item.ticker = colEls[j].textContent.trim(); \
                } else if (j == 2) { \
                    item.company_name = colEls[j].textContent.trim(); \
                } else if (j == 8) { \
                    item.days_old = colEls[j].textContent.trim(); \
                }\
            } \
            items.push(item); \
        } \
        JSON.stringify(items); \
    ",
    QWebEngineScript::ApplicationWorld,
    [this](const QVariant &items) {
        QJsonParseError jsonParseError;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(items.toByteArray(), &jsonParseError);

        QList<Ipo> ipos;
        foreach (const QJsonValue &item, jsonDocument.array()) {
            Ipo ipo;
            QJsonObject ipoJsonObj = item.toObject();

            ipo.ticker = ipoJsonObj["ticker"].toString();
            ipo.stock_exchange = "OTC Markets";
            ipo.company_name = ipoJsonObj["company_name"].toString();
            ipo.status = IPO_STATUS_PRICED;
            ipo.priced_date = QDateTime::currentDateTime().addDays(-ipoJsonObj["days_old"].toString().toInt());
            ipo.region = IPO_REGION_NA_USA;

            ipos.append(ipo);
        }
        emit ipoInfoObtainedSignal(&ipos);
    });
}

void DataSourceOtcbbSwingtradebot::queryData()
{
    QUrl url = QUrl("https://otcbb.swingtradebot.com/equities/recent-ipos");

    QWebEnginePage *page = new QWebEnginePage(this);
    // No need to load images
    page->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, false);
    // JS needs to be disabled to properly scrape contents on this page
    page->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    page->load(url);
    connect(page, &QWebEnginePage::loadFinished, this, [this, page] {
        parseMainPage(page);
    });
}
