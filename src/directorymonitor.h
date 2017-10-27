/*
    This file is part of DocScan.

    DocScan is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DocScan is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DocScan.  If not, see <https://www.gnu.org/licenses/>.


    Copyright (2017) Thomas Fischer <thomas.fischer@his.se>, senior
    lecturer at University of Skövde, as part of the LIM-IT project.

 */

#ifndef DIRECTORYMONITOR_H
#define DIRECTORYMONITOR_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QSet>

#include "filefinder.h"

/**
 * Continuously monitor a file system tree starting from
 * a base directory and signal found files as if they were
 * found URLs.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class DirectoryMonitor : public FileFinder
{
    Q_OBJECT

public:
    /**
     * Initialize directory monitor. Actual monitoring will only start
     * at first invocation of @see startSearch.
     * @param timeLimitMSeconds Duration in milliseconds after which directory monitor will stop
     * @param filters List of filename filters passed to QDir::entryList(..)
     * @param baseDir Directory to monitor
     * @param parent Used in QObject hierarchy
     */
    explicit DirectoryMonitor(int timeLimitMSeconds, const QStringList &filters, const QString &baseDir, QObject *parent = nullptr);

    virtual void startSearch(int numExpectedHits);
    virtual bool isAlive();

private slots:
    void directoryChanged(const QString &path);
    void rescanBaseDirectory();

private:
    bool m_alive;

    int m_numExpectedHits, m_timeLimitMSeconds;
    const QStringList &m_filters;
    QString m_baseDir;

    QFileSystemWatcher m_fileSystemWatcher;
    QTimer m_timerWaitForSettledDir;

    QSet<QUrl> m_knownFiles;
};

#endif // DIRECTORYMONITOR_H
