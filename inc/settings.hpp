#pragma once

#include <QSettings>

class Settings
{
public:
    explicit Settings();
    ~Settings();

    QString get(QString key);
    void set(QString key, QString value);

    QString filePath();

private:
    QSettings *settings;
};
