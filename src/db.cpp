#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

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
#define DB_IPO_REGION_OC_AUSTRALIA_STR   "as"
#define DB_IPO_REGION_SA_INDIA_STR       "in"
#define DB_IPO_REGION_GLOBAL_STR         "global"
#define DB_IPO_REGION_UNKNOWN_STR        "unknown"

#define DB_LOG_SQL_QUERY_ERROR(q) qDebug().noquote() << QString("Error in %1:").arg(Q_FUNC_INFO) << q.lastError().text()

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
    { IPO_REGION_OC_AUSTRALIA,   DB_IPO_REGION_OC_AUSTRALIA_STR },
    { IPO_REGION_SA_INDIA,       DB_IPO_REGION_SA_INDIA_STR },
    { IPO_REGION_GLOBAL,         DB_IPO_REGION_GLOBAL_STR },
    { IPO_REGION_UNKNOWN,        DB_IPO_REGION_UNKNOWN_STR },
};

namespace {

QVariant dateToDb(const QDateTime& date)
{
    return date.isValid() ? QVariant(date.toString(Qt::ISODate)) : QVariant();
}

QDateTime dbToDate(const QVariant& value)
{
    if (value.isNull()) {
        return QDateTime();
    }

    const QString str = value.toString();
    QDateTime date = QDateTime::fromString(str, Qt::ISODate);
    if (!date.isValid()) {
        date = QDateTime::fromString(str, Qt::ISODateWithMs);
    }

    return date;
}

// Progression of non-withdrawn IPO statuses
int statusRank(const IpoStatus status)
{
    switch (status) {
        case IPO_STATUS_FILED:    return 1;
        case IPO_STATUS_EXPECTED: return 2;
        case IPO_STATUS_PRICED:   return 3;
        default:                  return 0;
    }
}

QVariant strToDb(const QString& str)
{
    return str.isEmpty() ? QVariant() : QVariant(str);
}

}

Db::Db(const QString dbDirPath, QObject* parent) : QObject(parent)
{
    {
        const QDir dbDir(dbDirPath);
        if (!dbDir.exists()) {
            dbDir.mkpath(dbDirPath);
        }
    }

    connectionName = QString(PROG_NAME "-db-%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase sqlDb = QSqlDatabase::addDatabase(DB_TYPE, connectionName);
    sqlDb.setDatabaseName(dbDirPath + QDir::separator() + PROG_NAME ".sqlite");

    if (!sqlDb.open()) {
        qWarning().noquote() << "Error: database connection failed:" << sqlDb.lastError().text();
    } else {
        createTables();
    }
}

Db::~Db()
{
    {
        QSqlDatabase sqlDb = QSqlDatabase::database(connectionName, false);
        if (sqlDb.isOpen()) {
            sqlDb.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
}

void Db::createTables()
{
    QSqlQuery query(QSqlDatabase::database(connectionName));

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

    if (!query.exec()) {
        DB_LOG_SQL_QUERY_ERROR(query);
    }
}

int Db::insertRecord(Ipo* ipo)
{
    QSqlQuery query(QSqlDatabase::database(connectionName));

    query.prepare(
        "INSERT INTO "
            DB_TABLE_NAME
        " ("
            "is_important,"
            "last_notification,"

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
            ":is_important,"
            ":last_notification,"

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

    query.bindValue(":is_important", ipo->is_important);
    query.bindValue(":last_notification", ipo->last_notification);

    query.bindValue(":filed_date", dateToDb(ipo->filed_date));
    query.bindValue(":expected_date", dateToDb(ipo->expected_date));
    query.bindValue(":priced_date", dateToDb(ipo->priced_date));
    query.bindValue(":withdrawn_date", dateToDb(ipo->withdrawn_date));

    query.bindValue(":company_name", ipo->company_name);
    query.bindValue(":ticker", strToDb(ipo->ticker));
    query.bindValue(":stock_exchange", strToDb(ipo->stock_exchange));
    query.bindValue(":status", ipoStatusToIpoStatusCodeStr(ipo->status));

    query.bindValue(":company_website", strToDb(ipo->company_website.toString()));
    query.bindValue(":market_sector", strToDb(ipo->market_sector));
    query.bindValue(":region", ipoRegionToDbIpoRegionStr(ipo->region));

    query.bindValue(":sources", ipo->sources.join(","));

    query.bindValue(":log", ipo->log);

    if (query.exec()) {
        return query.lastInsertId().toInt();
    }

    DB_LOG_SQL_QUERY_ERROR(query);

    return 0;
}

const QList<Ipo>& Db::ipos() const
{
    return allIpos;
}

const char* Db::ipoRegionToDbIpoRegionStr(const IpoRegion ipoRegion)
{
    for (const IntKeyStrValPair &regionIntKeyStrValPair : regionEnumRegionStrTable) {
        if (regionIntKeyStrValPair.key == ipoRegion) {
            return regionIntKeyStrValPair.str;
        }
    }

    return DB_IPO_REGION_UNKNOWN_STR;
}

IpoRegion Db::dbIpoRegionStrToIpoRegion(const QString& ipoRegionStr)
{
    for (const IntKeyStrValPair &regionIntKeyStrValPair : regionEnumRegionStrTable) {
        if (ipoRegionStr == QLatin1String(regionIntKeyStrValPair.str)) {
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

IpoStatus Db::ipoStatusCodeStrToIpoStatus(const QString& ipoStatusCodeStr)
{
    if (ipoStatusCodeStr == DB_IPO_STATUS_EXPECTED_STR) {
        return IPO_STATUS_EXPECTED;
    } else if (ipoStatusCodeStr == DB_IPO_STATUS_PRICED_STR) {
        return IPO_STATUS_PRICED;
    } else if (ipoStatusCodeStr == DB_IPO_STATUS_WITHDRAWN_STR) {
        return IPO_STATUS_WITHDRAWN;
    } else if (ipoStatusCodeStr == DB_IPO_STATUS_FILED_STR) {
        return IPO_STATUS_FILED;
    }

    return IPO_STATUS_UNKNOWN;
}

QString Db::normalizeCompanyName(const QString& companyName)
{
    // Legal-form suffixes that various data sources may or may not include
    static const QStringList suffixes = {
        "inc", "incorporated", "corp", "corporation", "co", "company", "ltd", "limited",
        "llc", "lp", "plc", "sa", "nv", "ag", "se", "asa", "ab", "spa", "kk", "bhd", "pjsc",
        "holding", "holdings", "group",
    };
    static const QRegularExpression nonWordRe("[^\\w]+", QRegularExpression::UseUnicodePropertiesOption);

    QString name = companyName.toLower();
    name.replace("（株）", " ").replace("(株)", " ").replace("株式会社", " ");
    name.replace("&", " and ");
    // Turn "S.A." into "sa", "N.V." into "nv", etc
    name.remove('.');
    name.replace(nonWordRe, " ");

    QStringList words = name.split(' ', Qt::SkipEmptyParts);
    while (words.size() > 1 && suffixes.contains(words.last())) {
        words.removeLast();
    }
    while (words.size() > 1 && words.first() == "the") {
        words.removeFirst();
    }

    return words.join(' ');
}

bool Db::checkIfSameIpo(const Ipo* ipo1, const Ipo* ipo2)
{
    // Regions must not contradict each other
    if (ipo1->region != IPO_REGION_UNKNOWN && ipo2->region != IPO_REGION_UNKNOWN && ipo1->region != ipo2->region) {
        return false;
    }

    // Same ticker within the same region
    if (!ipo1->ticker.isEmpty() && ipo1->region == ipo2->region &&
        ipo1->ticker.compare(ipo2->ticker, Qt::CaseInsensitive) == 0) {
        return true;
    }

    const QString name1 = normalizeCompanyName(ipo1->company_name);

    return !name1.isEmpty() && name1 == normalizeCompanyName(ipo2->company_name);
}

bool Db::mergeIpo(Ipo* i, const Ipo* retrievedIpo, const QString& dataSourceName)
{
    bool somethingNew = false;

    // Dates (only ever overwrite with valid values, data sources often know only one of them)
    if (retrievedIpo->filed_date.isValid() && retrievedIpo->filed_date != i->filed_date) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated filed_date from “%1” to “%2” using data source %3").arg(
            i->filed_date.toString(Qt::ISODate),
            retrievedIpo->filed_date.toString(Qt::ISODate),
            dataSourceName
        ));
        i->filed_date = retrievedIpo->filed_date;
    }
    if (retrievedIpo->expected_date.isValid() && retrievedIpo->expected_date != i->expected_date) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated expected_date from “%1” to “%2” using data source %3").arg(
            i->expected_date.toString(Qt::ISODate),
            retrievedIpo->expected_date.toString(Qt::ISODate),
            dataSourceName
        ));
        i->expected_date = retrievedIpo->expected_date;
        // A new expected date means a previous notification is no longer relevant
        i->last_notification = 0;
    }
    if (retrievedIpo->priced_date.isValid() && retrievedIpo->priced_date != i->priced_date) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated priced_date from “%1” to “%2” using data source %3").arg(
            i->priced_date.toString(Qt::ISODate),
            retrievedIpo->priced_date.toString(Qt::ISODate),
            dataSourceName
        ));
        i->priced_date = retrievedIpo->priced_date;
    }
    if (retrievedIpo->withdrawn_date.isValid() && retrievedIpo->withdrawn_date != i->withdrawn_date) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated withdrawn_date from “%1” to “%2” using data source %3").arg(
            i->withdrawn_date.toString(Qt::ISODate),
            retrievedIpo->withdrawn_date.toString(Qt::ISODate),
            dataSourceName
        ));
        i->withdrawn_date = retrievedIpo->withdrawn_date;
    }

    // Status (filed -> expected -> priced, or withdrawn at any point)
    {
        IpoStatus newStatus = i->status;

        if (retrievedIpo->status == IPO_STATUS_WITHDRAWN || i->withdrawn_date.isValid()) {
            newStatus = IPO_STATUS_WITHDRAWN;
        } else if (i->status != IPO_STATUS_WITHDRAWN) {
            IpoStatus derived = IPO_STATUS_UNKNOWN;
            if (i->priced_date.isValid()) {
                derived = IPO_STATUS_PRICED;
            } else if (i->expected_date.isValid()) {
                derived = IPO_STATUS_EXPECTED;
            } else if (i->filed_date.isValid()) {
                derived = IPO_STATUS_FILED;
            }
            if (statusRank(retrievedIpo->status) > statusRank(derived)) {
                derived = retrievedIpo->status;
            }
            // Never move backwards (e.g. from priced to expected)
            if (statusRank(derived) > statusRank(i->status)) {
                newStatus = derived;
            }
        }

        if (newStatus != i->status) {
            somethingNew = true;
            writeIntoLog(i, QString("Updated status from “%1” to “%2” using data source %3").arg(
                ipoStatusToIpoStatusCodeStr(i->status),
                ipoStatusToIpoStatusCodeStr(newStatus),
                dataSourceName
            ));
            i->status = newStatus;
        }
    }

    if (!retrievedIpo->ticker.isEmpty() && retrievedIpo->ticker != i->ticker) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated ticker from “%1” to “%2” using data source %3").arg(
            i->ticker,
            retrievedIpo->ticker,
            dataSourceName
        ));
        i->ticker = retrievedIpo->ticker;
    }
    if (!retrievedIpo->stock_exchange.isEmpty() && retrievedIpo->stock_exchange != i->stock_exchange) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated stock_exchange from “%1” to “%2” using data source %3").arg(
            i->stock_exchange,
            retrievedIpo->stock_exchange,
            dataSourceName
        ));
        i->stock_exchange = retrievedIpo->stock_exchange;
    }
    if (!retrievedIpo->market_sector.isEmpty() && retrievedIpo->market_sector != i->market_sector) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated market_sector from “%1” to “%2” using data source %3").arg(
            i->market_sector,
            retrievedIpo->market_sector,
            dataSourceName
        ));
        i->market_sector = retrievedIpo->market_sector;
    }
    if (!retrievedIpo->company_website.isEmpty() && retrievedIpo->company_website != i->company_website) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated company_website from “%1” to “%2” using data source %3").arg(
            i->company_website.toString(),
            retrievedIpo->company_website.toString(),
            dataSourceName
        ));
        i->company_website = retrievedIpo->company_website;
    }
    if (retrievedIpo->region != IPO_REGION_UNKNOWN && retrievedIpo->region != i->region) {
        somethingNew = true;
        writeIntoLog(i, QString("Updated region from “%1” to “%2” using data source %3").arg(
            ipoRegionToDbIpoRegionStr(i->region),
            ipoRegionToDbIpoRegionStr(retrievedIpo->region),
            dataSourceName
        ));
        i->region = retrievedIpo->region;
    }

    if (!i->sources.contains(dataSourceName)) {
        somethingNew = true;
        writeIntoLog(i, QString("Added data source “%1” to sources").arg(dataSourceName));
        i->sources << dataSourceName;
    }

    return somethingNew;
}

void Db::processNewlyObtainedData(const QList<Ipo>* retrievedIpos, const QString* dataSourceName)
{
    QList<Ipo> changedIpos;

    for (const Ipo& retrievedIpo : *retrievedIpos) {
        if (retrievedIpo.company_name.trimmed().isEmpty()) {
            continue;
        }

        // Loop through known IPOs and see if this company's IPO is already in the database
        QList<Ipo>::iterator i = allIpos.begin();
        for (; i != allIpos.end(); ++i) {
            if (checkIfSameIpo(&*i, &retrievedIpo)) {
                break;
            }
        }

        if (i != allIpos.end()) {
            // Update existing record (in case there's something new to it)
            if (mergeIpo(&*i, &retrievedIpo, *dataSourceName)) {
                updateRecord(&*i);
                changedIpos.append(*i);
            }
        } else {
            // Insert new record
            Ipo newIpo = retrievedIpo;
            newIpo.id = 0;
            newIpo.sources = QStringList() << *dataSourceName;
            writeIntoLog(&newIpo, QString("Created using data obtained from data source %1").arg(*dataSourceName));

            const int newIpoRecordId = insertRecord(&newIpo);
            if (newIpoRecordId > 0) {
                newIpo.id = newIpoRecordId;
                allIpos.append(newIpo);
                changedIpos.append(newIpo);
            }
        }
    }

    if (!changedIpos.isEmpty()) {
        emit ipoRecordsRetrievedSignal(changedIpos);
    }
}

void Db::readRecords()
{
    QSqlQuery query(QSqlDatabase::database(connectionName));

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

    if (!query.exec()) {
        DB_LOG_SQL_QUERY_ERROR(query);
        return;
    }

    allIpos.clear();

    while (query.next()) {
        Ipo ipo;

        ipo.id = query.value(0).toInt();

        ipo.is_important = query.value(1).toBool();
        ipo.last_notification = query.value(2).toInt();

        ipo.filed_date = dbToDate(query.value(3));
        ipo.expected_date = dbToDate(query.value(4));
        ipo.priced_date = dbToDate(query.value(5));
        ipo.withdrawn_date = dbToDate(query.value(6));

        ipo.company_name = query.value(7).toString();
        ipo.ticker = query.value(8).toString();
        ipo.stock_exchange = query.value(9).toString();
        ipo.status = ipoStatusCodeStrToIpoStatus(query.value(10).toString());

        ipo.company_website = QUrl(query.value(11).toString());
        ipo.market_sector = query.value(12).toString();
        ipo.region = dbIpoRegionStrToIpoRegion(query.value(13).toString());

        const QString sourcesString = query.value(14).toString();
        if (!sourcesString.isEmpty()) {
            ipo.sources = sourcesString.split(",");
        }

        ipo.notes = query.value(15).toString();
        ipo.meta = query.value(16).toString();
        ipo.log = query.value(17).toString();

        // Store duplicate of this IPO record in memory
        allIpos.append(ipo);
    }

    if (!allIpos.isEmpty()) {
        emit ipoRecordsRetrievedSignal(allIpos);
    }
}

void Db::setLastNotification(int ipoId, int hoursLeft)
{
    for (Ipo& ipo : allIpos) {
        if (ipo.id == ipoId) {
            ipo.last_notification = hoursLeft;
            updateRecord(&ipo);
            break;
        }
    }
}

void Db::toggleImportant(int ipoId)
{
    for (Ipo& ipo : allIpos) {
        if (ipo.id == ipoId) {
            ipo.is_important = !ipo.is_important;
            // Start reminders over when (re-)flagged
            ipo.last_notification = 0;
            updateRecord(&ipo);
            break;
        }
    }
}

void Db::updateRecord(Ipo* ipo)
{
    QSqlQuery query(QSqlDatabase::database(connectionName));

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
        ";"
    );

    query.bindValue(":is_important", ipo->is_important);
    query.bindValue(":last_notification", ipo->last_notification);

    query.bindValue(":filed_date", dateToDb(ipo->filed_date));
    query.bindValue(":expected_date", dateToDb(ipo->expected_date));
    query.bindValue(":priced_date", dateToDb(ipo->priced_date));
    query.bindValue(":withdrawn_date", dateToDb(ipo->withdrawn_date));

    query.bindValue(":company_name", ipo->company_name);
    query.bindValue(":ticker", strToDb(ipo->ticker));
    query.bindValue(":stock_exchange", strToDb(ipo->stock_exchange));
    query.bindValue(":status", ipoStatusToIpoStatusCodeStr(ipo->status));

    query.bindValue(":company_website", strToDb(ipo->company_website.toString()));
    query.bindValue(":market_sector", strToDb(ipo->market_sector));
    query.bindValue(":region", ipoRegionToDbIpoRegionStr(ipo->region));

    query.bindValue(":sources", ipo->sources.join(","));

    query.bindValue(":notes", ipo->notes);
    query.bindValue(":meta", ipo->meta);
    query.bindValue(":log", ipo->log);

    query.bindValue(":id", ipo->id);

    if (!query.exec()) {
        DB_LOG_SQL_QUERY_ERROR(query);
    }
}

void Db::writeIntoLog(Ipo* ipo, const QString message)
{
    ipo->log += QString("[%1] %2\n").arg(
        QDateTime::currentDateTime().toString(Qt::ISODate),
        message
    );
}
