#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

#include "ipo.hpp"

class Db
{
public:
    Db(const QString filePath);
    ~Db();

    bool processNewlyObtainedData(const Ipo *retrievedIpo, const QString *dataSourceName);
    void toggleImportant(int ipoId);

    QList<Ipo> ipos;

private:
    void createTables();
    int insertRecord(Ipo *ipo);
    static QString ipoRegionEnumToIpoRegionStr(const IpoRegion ipoRegionEnum);
    static IpoRegion ipoRegionStrToIpoStatusEnum(const QString ipoRegionStr);
    static QString ipoStatusToIpoStatusCodeStr(const IpoStatus ipoStatusEnum);
    static IpoStatus ipoStatusCodeStrToIpoStatus(const QString ipoStatusStr);
    static bool isSameIpo(const Ipo *ipo1, const Ipo *ipo2);
    void readRecords();
    static bool sortFn(const Ipo &ipo1, const Ipo &ipo2);
    void sortRecords();
    void updateRecord(Ipo *ipo);
    static void writeIntoLog(Ipo *ipo, const QString message);

    QSqlDatabase db;
};
