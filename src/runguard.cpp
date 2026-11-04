#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

#include "runguard.hpp"

namespace
{

QString lockFilePath(const QString& key)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (dir.isEmpty()) {
        dir = QDir::tempPath();
    }

    const QString hash = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex());

    return dir + QDir::separator() + hash + ".lock";
}

}

RunGuard::RunGuard(const QString& key) : lockFile(lockFilePath(key))
{
    // Never consider a lock held by a live process stale
    lockFile.setStaleLockTime(0);
}

RunGuard::~RunGuard()
{
    release();
}

bool RunGuard::tryToRun()
{
    return lockFile.tryLock(0);
}

void RunGuard::release()
{
    if (lockFile.isLocked()) {
        lockFile.unlock();
    }
}
