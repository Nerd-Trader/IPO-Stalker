#pragma once

#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>

#include "ipo.hpp"

class Db : public QObject
{
    Q_OBJECT

public:
    explicit Db(const QString dbDirPath, QObject* parent = nullptr);
    ~Db();

    const QList<Ipo>& ipos() const;
    void processNewlyObtainedData(const QList<Ipo>* retrievedIpos, const QString* dataSourceName);
    void readRecords();
    void setLastNotification(int ipoId, int hoursLeft);
    void toggleImportant(int ipoId);

    static bool checkIfSameIpo(const Ipo* ipo1, const Ipo* ipo2);
    static QString normalizeCompanyName(const QString& companyName);
    static bool mergeIpo(Ipo* existing, const Ipo* retrieved, const QString& dataSourceName);

signals:
    void ipoRecordsRetrievedSignal(const QList<Ipo>& ipos);

private:
    void createTables();
    int insertRecord(Ipo* ipo);
    static const char* ipoRegionToDbIpoRegionStr(const IpoRegion ipoRegion);
    static IpoRegion dbIpoRegionStrToIpoRegion(const QString& ipoRegionStr);
    static const char* ipoStatusToIpoStatusCodeStr(const IpoStatus ipoStatus);
    static IpoStatus ipoStatusCodeStrToIpoStatus(const QString& ipoStatusStr);
    void updateRecord(Ipo* ipo);
    static void writeIntoLog(Ipo* ipo, const QString message);

    QList<Ipo> allIpos;
    QString connectionName;
};
