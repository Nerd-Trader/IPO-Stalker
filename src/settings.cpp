#include "settings.hpp"

Settings::Settings()
{
    settings = new QSettings(QSettings::IniFormat,
                             QSettings::UserScope,
                             PROG_NAME, PROG_NAME,
                             nullptr);
}

Settings::~Settings()
{
}

QString Settings::get(QString key)
{
    if (settings->contains(key)) {
        return settings->value(key).toString();
    } else {
        return QString();
    }
}

void Settings::set(QString key, QString value)
{
    settings->setValue(key, value);
    settings->sync();
}

QString Settings::filePath()
{
    return settings->fileName();
}
