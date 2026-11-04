#pragma once

#include <QLockFile>
#include <QString>

// Ensures only one instance of the program runs per user.
// Uses a lock file, which (unlike shared memory) can't be left dangling after a crash.
class RunGuard
{
public:
    explicit RunGuard(const QString& key);
    ~RunGuard();

    bool tryToRun();
    void release();

private:
    QLockFile lockFile;

    Q_DISABLE_COPY(RunGuard)
};
