#pragma once

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QThread>

#include "ipo.hpp"

class Db : public QThread
{
    Q_OBJECT

public:
    Db(const QString* dbDirPath = nullptr);
    ~Db();

    void processNewlyObtainedData(const QList<Ipo>* retrievedIpos, const QString* dataSourceName);
    void toggleImportant(int ipoId);

public slots:
    void readDataSlot();

signals:
    void ipoRecordInsertedSignal(const Ipo* ipo);
    void ipoRecordUpdatedSignal(const Ipo* ipo);
    void ipoRecordsRetrievedSignal(const QList<Ipo>* ipos);

private:
    static bool checkIfSameIpo(const Ipo* ipo1, const Ipo* ipo2);
    void createTables();
    int insertRecord(Ipo* ipo);
    static QString ipoRegionEnumToIpoRegionStr(const IpoRegion ipoRegionEnum);
    static IpoRegion ipoRegionStrToIpoStatusEnum(const QString ipoRegionStr);
    static QString ipoStatusToIpoStatusCodeStr(const IpoStatus ipoStatusEnum);
    static IpoStatus ipoStatusCodeStrToIpoStatus(const QString ipoStatusStr);
    void readRecords();
    void updateRecord(Ipo* ipo);
    static void writeIntoLog(Ipo* ipo, const QString message);

    QList<Ipo> allIpos;
    QSqlDatabase sqlDb;
};
