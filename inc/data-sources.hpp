#include <QTimer>

#include "data-sources/ipo-cal-appspot.hpp"

class DataSources : public QObject
{
    Q_OBJECT

public:
    DataSources();

public slots:
    void queryJapan();

private:
    QTimer *timer;
    DataSourceIpoCalAppSpot *dataSourceJapan = new DataSourceIpoCalAppSpot();
};
