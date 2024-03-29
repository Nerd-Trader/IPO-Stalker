#include <QDateTime>
#include <QDebug>
#include <QDir>

#include "common.hpp"
#include "db.hpp"

#define DB_TYPE       "QSQLITE"
#define DB_TABLE_NAME "initial_offerings"

// These values must to be unique and never changed
#define DB_IPO_STATUS_FILED_STR     "fld"
#define DB_IPO_STATUS_EXPECTED_STR  "exp"
#define DB_IPO_STATUS_PRICED_STR    "lst"
#define DB_IPO_STATUS_WITHDRAWN_STR "wdr"
#define DB_IPO_STATUS_UNKNOWN_STR   "unk"

// These values must to be unique and never changed
// CIA notation, taken from https://country-code.cl
#define DB_IPO_REGION_EA_JAPAN_STR       "ja"
#define DB_IPO_REGION_EU_BELGIUM_STR     "be"
#define DB_IPO_REGION_EU_FRANCE_STR      "fr"
#define DB_IPO_REGION_EU_IRELAND_STR     "ei"
#define DB_IPO_REGION_EU_ITALY_STR       "it"
#define DB_IPO_REGION_EU_NETHERLANDS_STR "nl"
#define DB_IPO_REGION_EU_NORWAY_STR      "no"
#define DB_IPO_REGION_EU_PORTUGAL_STR    "po"
#define DB_IPO_REGION_EU_UK_STR          "uk"
#define DB_IPO_REGION_NA_CANADA_STR      "ca"
#define DB_IPO_REGION_NA_USA_STR         "us"
#define DB_IPO_REGION_SA_INDIA_STR       "in"
#define DB_IPO_REGION_GLOBAL_STR         "global"
#define DB_IPO_REGION_UNKNOWN_STR        "unknown"

#define DB_LOG_SQL_QUERY_ERROR(q) qDebug().noquote() << QString("Error in %1:").arg(Q_FUNC_INFO) << q.lastError().text();

const struct IntKeyStrValPair regionEnumRegionStrTable[] = {
    { IPO_REGION_EA_JAPAN,       DB_IPO_REGION_EA_JAPAN_STR },
    { IPO_REGION_EU_BELGIUM,     DB_IPO_REGION_EU_BELGIUM_STR },
    { IPO_REGION_EU_FRANCE,      DB_IPO_REGION_EU_FRANCE_STR },
    { IPO_REGION_EU_IRELAND,     DB_IPO_REGION_EU_IRELAND_STR },
    { IPO_REGION_EU_ITALY,       DB_IPO_REGION_EU_ITALY_STR },
    { IPO_REGION_EU_NETHERLANDS, DB_IPO_REGION_EU_NETHERLANDS_STR },
    { IPO_REGION_EU_NORWAY,      DB_IPO_REGION_EU_NORWAY_STR },
    { IPO_REGION_EU_PORTUGAL,    DB_IPO_REGION_EU_PORTUGAL_STR },
    { IPO_REGION_EU_UK,          DB_IPO_REGION_EU_UK_STR },
    { IPO_REGION_NA_CANADA,      DB_IPO_REGION_NA_CANADA_STR },
    { IPO_REGION_NA_USA,         DB_IPO_REGION_NA_USA_STR },
    { IPO_REGION_SA_INDIA,       DB_IPO_REGION_SA_INDIA_STR },
    { IPO_REGION_GLOBAL,         DB_IPO_REGION_GLOBAL_STR },
    { IPO_REGION_UNKNOWN,        DB_IPO_REGION_UNKNOWN_STR },
};

Db::Db(const QString dbDirPath) : QThread()
{
    sqlDb = QSqlDatabase::addDatabase(DB_TYPE);
    {
        const QDir dbDir(dbDirPath);
        if (!dbDir.exists()) {
            dbDir.mkpath(dbDirPath);
        }
    }
    const QString dbFilePath = dbDirPath + QDir::separator() + PROG_NAME ".sqlite";
    sqlDb.setDatabaseName(dbFilePath);

    // Initial run
    connect(this, SIGNAL(started()), this, SLOT(readDataSlot()));

    if (!sqlDb.open()) {
        qDebug().noquote() << "Error: database connection failed";
    } else {
        createTables();
    }
}

Db::~Db()
{
    sqlDb.close();
    delete &sqlDb;
}

void Db::createTables()
{
    QSqlQuery query(sqlDb);

    query.prepare(
        "CREATE TABLE IF NOT EXISTS "
            DB_TABLE_NAME
        "("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"

            "is_important BOOLEAN DEFAULT FALSE," // Flagged or not (the program should send notifications for flagged IPOs)
            "last_notification INT DEFAULT 0," // Minimal number of hours left before the IPO (e.g. 48, 24, 1)

            "filed_date DATETIME,"
            "expected_date DATETIME,"
            "priced_date DATETIME,"
            "withdrawn_date DATETIME,"

            "company_name VARCHAR,"
            "ticker VARCHAR,"
            "stock_exchange VARCHAR,"
            "status VARCHAR,"
            "type VARCHAR," // Equity, Debt, Direct Listing, etc

            "company_website VARCHAR,"
            "market_sector VARCHAR,"
            "region VARCHAR,"

            "sources VARCHAR," // Comma-separated names of data sources that were used to compose this record

            "notes TEXT," // Optional notes specified by user
            "meta TEXT," // Various information in serialized format
            "log TEXT" // This record's history (when was created and from what data source, what and when modified, etc)
        ")"
        ";"
    );

    query.exec();

#ifdef DEBUG
    if (!query.isActive()) {
        DB_LOG_SQL_QUERY_ERROR(query);
    }
#endif
}

int Db::insertRecord(Ipo* ipo)
{
    QSqlQuery query;

    query.prepare(
        "INSERT INTO "
            DB_TABLE_NAME
        " ("
            "filed_date,"
            "expected_date,"
            "priced_date,"
            "withdrawn_date,"

            "company_name,"
            "ticker,"
            "stock_exchange,"
            "status,"

            "company_website,"
            "market_sector,"
            "region,"

            "sources,"

            "log"
        ") "
        " VALUES "
        " ("
            ":filed_date,"
            ":expected_date,"
            ":priced_date,"
            ":withdrawn_date,"

            ":company_name,"
            ":ticker,"
            ":stock_exchange,"
            ":status,"

            ":company_website,"
            ":market_sector,"
            ":region,"

            ":sources,"

            ":log"
        ")"
        ";"
    );
    {
        query.bindValue(":filed_date", ipo->filed_date);
        query.bindValue(":expected_date", ipo->expected_date);
        query.bindValue(":priced_date", ipo->priced_date);
        query.bindValue(":withdrawn_date", ipo->withdrawn_date);

        query.bindValue(":company_name", ipo->company_name);
        if (ipo->ticker.size() > 0) {
            query.bindValue(":ticker", ipo->ticker);
        }
        query.bindValue(":stock_exchange", ipo->stock_exchange);
        query.bindValue(":status", ipoStatusToIpoStatusCodeStr(ipo->status));

        if (!ipo->company_website.isEmpty()) {
            query.bindValue(":company_website", ipo->company_website);
        }
        query.bindValue(":market_sector", ipo->market_sector);
        query.bindValue(":region", ipoRegionToDbIpoRegionStr(ipo->region));

        query.bindValue(":sources", ipo->sources.join(","));

        query.bindValue(":log", ipo->log);
    }

    if (query.exec()) {
        return query.lastInsertId().toInt();
    }
#ifdef DEBUG
    else {
        DB_LOG_SQL_QUERY_ERROR(query);
    }
#endif

    return 0;
}

const char* Db::ipoRegionToDbIpoRegionStr(const IpoRegion ipoRegion)
{
    for (const IntKeyStrValPair &regionIntKeyStrValPair : regionEnumRegionStrTable) {
        if (regionIntKeyStrValPair.key == ipoRegion) {
            return regionIntKeyStrValPair.str;
        }
    }

    return nullptr;
}

IpoRegion Db::dbIpoRegionStrToIpoStatus(const char* ipoRegionStr)
{
    for (const IntKeyStrValPair &regionIntKeyStrValPair : regionEnumRegionStrTable) {
        if (strcmp(regionIntKeyStrValPair.str, ipoRegionStr) == 0) {
            return (IpoRegion)regionIntKeyStrValPair.key;
        }
    }

    return IPO_REGION_UNKNOWN;
}

const char* Db::ipoStatusToIpoStatusCodeStr(const IpoStatus ipoStatus)
{
    switch (ipoStatus) {
        case IPO_STATUS_EXPECTED:
            return DB_IPO_STATUS_EXPECTED_STR;

        case IPO_STATUS_PRICED:
            return DB_IPO_STATUS_PRICED_STR;

        case IPO_STATUS_WITHDRAWN:
            return DB_IPO_STATUS_WITHDRAWN_STR;

        case IPO_STATUS_FILED:
            return DB_IPO_STATUS_FILED_STR;

        default:
            return DB_IPO_STATUS_UNKNOWN_STR;
    }
}

IpoStatus Db::ipoStatusCodeStrToIpoStatus(const char* ipoStatusCodeStr)
{
    if (strcmp(ipoStatusCodeStr, DB_IPO_STATUS_EXPECTED_STR) == 0) {
        return IPO_STATUS_EXPECTED;
    } else if (strcmp(ipoStatusCodeStr, DB_IPO_STATUS_PRICED_STR) == 0) {
        return IPO_STATUS_PRICED;
    } else if (strcmp(ipoStatusCodeStr, DB_IPO_STATUS_WITHDRAWN_STR) == 0) {
        return IPO_STATUS_WITHDRAWN;
    } else if (strcmp(ipoStatusCodeStr, DB_IPO_STATUS_FILED_STR) == 0) {
        return IPO_STATUS_FILED;
    }

    return IPO_STATUS_UNKNOWN;
}

bool Db::checkIfSameIpo(const Ipo* ipo1, const Ipo* ipo2)
{
    // TODO: this method needs to become much smarter
    return ipo1->company_name.toLower() == ipo2->company_name.toLower();
}

void Db::processNewlyObtainedData(const QList<Ipo>* retrievedIpos, const QString* dataSourceName)
{
    QList<Ipo> ipos;

    for (int j = 0; j < retrievedIpos->size(); ++j) {
        const Ipo* retrievedIpo = &retrievedIpos->at(j);

        bool alreadyInDb = false;
        bool somethingNew = false;

        // Loop through known IPOs and see if this company's IPO is already in the database
        QList<Ipo>::iterator i = allIpos.begin();
        for (; i != allIpos.end(); ++i) {
            // Skip if this existing IPO record is for some other company
            if (!checkIfSameIpo(&*i, &*retrievedIpo)) {
                continue;
            }

            alreadyInDb = true;

            if (retrievedIpo->filed_date != i->filed_date) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Updated filed_date from “%1” to “%2” using data source %3").arg(
                    i->filed_date.toString(Qt::ISODate),
                    retrievedIpo->filed_date.toString(Qt::ISODate),
                    *dataSourceName
                ));
                i->filed_date = retrievedIpo->filed_date;
                if (i->status == IPO_STATUS_UNKNOWN) {
                    i->status = IPO_STATUS_FILED;
                }
            }
            if (retrievedIpo->expected_date != i->expected_date) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Updated expected_date from “%1” to “%2” using data source %3").arg(
                    i->expected_date.toString(Qt::ISODate),
                    retrievedIpo->expected_date.toString(Qt::ISODate),
                    *dataSourceName
                ));
                i->expected_date = retrievedIpo->expected_date;
                if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED) {
                    i->status = IPO_STATUS_EXPECTED;
                }
            }
            if (retrievedIpo->priced_date != i->priced_date) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Updated priced_date from “%1” to “%2” using data source %3").arg(
                    i->priced_date.toString(Qt::ISODate),
                    retrievedIpo->priced_date.toString(Qt::ISODate),
                    *dataSourceName
                ));
                i->priced_date = retrievedIpo->priced_date;
                if (i->status == IPO_STATUS_UNKNOWN || i->status == IPO_STATUS_FILED || i->status == IPO_STATUS_EXPECTED) {
                    i->status = IPO_STATUS_PRICED;
                }
            }
            if (retrievedIpo->withdrawn_date != i->withdrawn_date) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Updated withdrawn_date from “%1” to “%2” using data source %3").arg(
                    i->withdrawn_date.toString(Qt::ISODate),
                    retrievedIpo->withdrawn_date.toString(Qt::ISODate),
                    *dataSourceName
                ));
                i->withdrawn_date = retrievedIpo->withdrawn_date;
                if (i->status != IPO_STATUS_WITHDRAWN) {
                    i->status = IPO_STATUS_WITHDRAWN;
                }
            }
            if (retrievedIpo->stock_exchange.size() > 0 && retrievedIpo->stock_exchange != i->stock_exchange) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Updated stock_exchange from “%1” to “%2” using data source %3").arg(
                    i->stock_exchange,
                    retrievedIpo->stock_exchange,
                    *dataSourceName
                ));
                i->stock_exchange = retrievedIpo->stock_exchange;
            }
            if (retrievedIpo->market_sector.size() > 0 && retrievedIpo->market_sector != i->market_sector) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Updated market_sector from “%1” to “%2” using data source %3").arg(
                    i->market_sector,
                    retrievedIpo->market_sector,
                    *dataSourceName
                ));
                i->market_sector = retrievedIpo->market_sector;
            }
            if (retrievedIpo->company_website.toString().size() > 0 && retrievedIpo->company_website != i->company_website) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Updated company_website from “%1” to “%2” using data source %3").arg(
                    i->company_website.toString(),
                    retrievedIpo->company_website.toString(),
                    *dataSourceName
                ));
                i->company_website = retrievedIpo->company_website;
            }
            if (retrievedIpo->region != IPO_REGION_UNKNOWN && retrievedIpo->region != i->region) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Updated region from “%1” to “%2” using data source %3").arg(
                    ipoRegionToDbIpoRegionStr(i->region),
                    ipoRegionToDbIpoRegionStr(retrievedIpo->region),
                    *dataSourceName
                ));
                i->region = retrievedIpo->region;
            }

            if (!i->sources.contains(*dataSourceName)) {
                somethingNew = true;
                writeIntoLog(&*i, QString("Added data source “%1” to sources").arg(*dataSourceName));
                i->sources << *dataSourceName;
            }

            // Found existing IPO, so no need to iterate further
            break;
        }

        if (alreadyInDb) {
            // Update existing record (in case there's something new to it)
            if (somethingNew) {
                updateRecord(&*i);

                // emit ipoRecordUpdatedSignal(&*i);
                ipos.append(*i);
            }
        } else {
            // Insert new record
            allIpos.append(*retrievedIpo);
            Ipo* newlyInsertedIpoRecord = &allIpos.last();
            newlyInsertedIpoRecord->sources << *dataSourceName;
            writeIntoLog(newlyInsertedIpoRecord, QString("Created using data obtained from data source %1").arg(*dataSourceName));
            const int newIpoRecordId = insertRecord(newlyInsertedIpoRecord);
            if (newIpoRecordId > 0) {
                newlyInsertedIpoRecord->id = newIpoRecordId;

                // emit ipoRecordInsertedSignal(newlyInsertedIpoRecord);
                ipos.append(*newlyInsertedIpoRecord);
            } else {
                // Remove IPO item from the in-memory list
                // in case the program failed to save it in the database
                allIpos.removeLast();
            }
        }
    }

    if (ipos.size() > 0) {
        emit ipoRecordsRetrievedSignal(&ipos);
    }
}

void Db::readDataSlot()
{
    readRecords();
}

void Db::readRecords()
{
    QSqlQuery query(sqlDb);

    query.prepare(
        "SELECT "
            "id," // 0

            "is_important," // 1
            "last_notification," // 2

            "filed_date," // 3
            "expected_date," // 4
            "priced_date," // 5
            "withdrawn_date," // 6

            "company_name," // 7
            "ticker," // 8
            "stock_exchange," // 9
            "status," // 10

            "company_website," // 11
            "market_sector," // 12
            "region," // 13

            "sources," // 14

            "notes," // 15
            "meta," // 16
            "log" // 17
        " FROM "
            DB_TABLE_NAME
        ";"
    );

    query.exec();

    if (!query.isActive()) {
#ifdef DEBUG
        DB_LOG_SQL_QUERY_ERROR(query);
#endif
        return;
    }

    while (query.next()) {
        Ipo ipo;

        ipo.id = query.value(0).toInt();

        ipo.is_important = query.value(1).toBool();
        ipo.last_notification = query.value(2).toInt();

        if (query.value(3).Size > 0)
            ipo.filed_date = query.value(3).toDateTime();
        if (query.value(4).Size > 0)
            ipo.expected_date = query.value(4).toDateTime();
        if (query.value(5).Size > 0)
            ipo.priced_date = query.value(5).toDateTime();
        if (query.value(6).Size > 0)
            ipo.withdrawn_date = query.value(6).toDateTime();

        ipo.company_name = query.value(7).toString();
        ipo.ticker = query.value(8).toString();
        ipo.stock_exchange = query.value(9).toString();
        ipo.status = ipoStatusCodeStrToIpoStatus(query.value(10).toString().toStdString().c_str());

        ipo.company_website = query.value(11).toString();
        ipo.market_sector = query.value(12).toString();
        ipo.region = dbIpoRegionStrToIpoStatus(query.value(13).toString().toStdString().c_str());

        const QString sourcesString = query.value(14).toString();
        if (sourcesString.size() > 0) {
            ipo.sources = query.value(14).toString().split(",");
        }

        ipo.notes = query.value(15).toString();
        ipo.meta = query.value(16).toString();
        ipo.log = query.value(17).toString();

        // Store duplicate of this IPO record in memory
        allIpos.append(ipo);
    }

    if (allIpos.size() > 0) {
        emit ipoRecordsRetrievedSignal(&allIpos);
    }
}

void Db::toggleImportant(int ipoId)
{
    QList<Ipo>::iterator i = allIpos.begin();
    for (; i != allIpos.end(); ++i) {
        if (i->id == ipoId) {
            i->is_important = !i->is_important;
            updateRecord(&*i);
            break;
        }
    }
}

void Db::updateRecord(Ipo* ipo)
{
    QSqlQuery query(sqlDb);

    query.prepare(
        "UPDATE "
            DB_TABLE_NAME
        " SET "
            "is_important = :is_important,"
            "last_notification = :last_notification,"

            "filed_date = :filed_date,"
            "expected_date = :expected_date,"
            "priced_date = :priced_date,"
            "withdrawn_date = :withdrawn_date,"

            "company_name = :company_name,"
            "ticker = :ticker,"
            "stock_exchange = :stock_exchange,"
            "status = :status,"

            "company_website = :company_website,"
            "market_sector = :market_sector,"
            "region = :region,"

            "sources = :sources,"

            "notes = :notes,"
            "meta = :meta,"
            "log = :log"
        " WHERE "
            "id = :id"
        " AND "
        "("
            "is_important IS NOT :is_important"
            " OR "
            "last_notification IS NOT :last_notification"

            " OR "
            "filed_date IS NOT :filed_date"
            " OR "
            "expected_date IS NOT :expected_date"
            " OR "
            "priced_date IS NOT :priced_date"
            " OR "
            "withdrawn_date IS NOT :withdrawn_date"

            " OR "
            "company_name IS NOT :company_name"
            " OR "
            "ticker IS NOT :ticker"
            " OR "
            "stock_exchange IS NOT :stock_exchange"
            " OR "
            "status IS NOT :status"

            " OR "
            "company_website IS NOT :company_website"
            " OR "
            "market_sector IS NOT :market_sector"
            " OR "
            "region IS NOT :region"

            " OR "
            "sources IS NOT :sources"

            " OR "
            "notes IS NOT :notes"
            " OR "
            "meta IS NOT :meta"
            " OR "
            "log IS NOT :log"
        ")"
        ";"
    );
    {
        query.bindValue(":id", ipo->id);

        query.bindValue(":is_important", ipo->is_important);
        query.bindValue(":last_notification", ipo->last_notification);

        query.bindValue(":filed_date", ipo->filed_date);
        query.bindValue(":expected_date", ipo->expected_date);
        query.bindValue(":priced_date", ipo->priced_date);
        query.bindValue(":withdrawn_date", ipo->withdrawn_date);

        query.bindValue(":company_name", ipo->company_name);
        if (!ipo->ticker.isEmpty()) {
            query.bindValue(":ticker", ipo->ticker);
        }
        query.bindValue(":stock_exchange", ipo->stock_exchange);
        query.bindValue(":status", ipoStatusToIpoStatusCodeStr(ipo->status));

        if (!ipo->company_website.isEmpty()) {
            query.bindValue(":company_website", ipo->company_website);
        }
        query.bindValue(":market_sector", ipo->market_sector);
        query.bindValue(":region", ipoRegionToDbIpoRegionStr(ipo->region));

        query.bindValue(":sources", ipo->sources.join(","));

        query.bindValue(":notes", ipo->notes);
        query.bindValue(":meta", ipo->meta);
        query.bindValue(":log", ipo->log);
    }

    query.exec();

#ifdef DEBUG
    if (!query.isActive()) {
        DB_LOG_SQL_QUERY_ERROR(query);
    }
#endif
}

void Db::writeIntoLog(Ipo* ipo, const QString message)
{
    ipo->log += QString("[%1] %2\n").arg(
        QDateTime::currentDateTime().toString(Qt::ISODate),
        message
    );
}
