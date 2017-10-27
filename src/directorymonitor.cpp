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

#include "directorymonitor.h"

#include <QDir>
#include <QDebug>

#include "general.h"

DirectoryMonitor::DirectoryMonitor(int timeLimitMSeconds, const QStringList &filters, const QString &baseDir, QObject *parent)
    : FileFinder(parent), m_alive(false), m_numExpectedHits(-1), m_timeLimitMSeconds(timeLimitMSeconds), m_filters(filters), m_baseDir(baseDir), m_fileSystemWatcher(QStringList() << baseDir, this), m_timerWaitForSettledDir(this)
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());
    connect(&m_timerWaitForSettledDir, &QTimer::timeout, this, &DirectoryMonitor::rescanBaseDirectory);
}

void DirectoryMonitor::startSearch(int numExpectedHits)
{
    static bool firstInvocation = true;
    if (firstInvocation) {
        m_alive = true;
        qDebug() << "Starting timeout of " << (m_timeLimitMSeconds / 1000) << " seconds for watching " << m_baseDir;
        QTimer::singleShot(m_timeLimitMSeconds, [this]() {
            qDebug() << "Timeout of " << (m_timeLimitMSeconds / 1000) << " seconds for watching " << m_baseDir;
            m_fileSystemWatcher.removePath(m_baseDir);
            m_alive = false;
        });
        connect(&m_fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &DirectoryMonitor::directoryChanged);
    } else
        qDebug() << "Repeated invocation, numExpectedHits=" << numExpectedHits;

    m_numExpectedHits = numExpectedHits;
    QStringList queue = QStringList() << m_baseDir;
    int hits = 0;

    while (hits < numExpectedHits && !queue.isEmpty()) {
        QDir dir = QDir(queue.first());
        queue.removeFirst();

        const QStringList files = dir.entryList(m_filters, QDir::Files, QDir::Name | QDir::IgnoreCase);
        for (const QString &filename : files) {
            const QUrl url = QUrl::fromLocalFile(dir.absolutePath() + QDir::separator() + filename);
            if (m_knownFiles.contains(url))
                continue;
            else
                m_knownFiles.insert(url);
            emit report(objectName(), QString(QStringLiteral("<filefinder event=\"hit\" href=\"%1\" />\n")).arg(DocScan::xmlify(url.toString())));
            emit foundUrl(url);
            ++hits;
            if (hits >= numExpectedHits) break;
        }

        const QStringList subdirEntries = dir.entryList(QDir::Dirs);
        for (const QString &subdir : subdirEntries) {
            if (subdir != QStringLiteral(".") && subdir != QStringLiteral("..")) {
                queue.append(dir.absolutePath() + QDir::separator() + subdir);
            }
        }
    }

    if (firstInvocation || hits > 0)
        emit report(objectName(), QString(QStringLiteral("<filesystemscan filter=\"%3\" directory=\"%2\" numresults=\"%1\" />\n")).arg(QString::number(hits), DocScan::xmlify(QDir(m_baseDir).absolutePath()), DocScan::xmlify(m_filters.join(QChar('|')))));

    m_numExpectedHits -= hits;
    if (m_numExpectedHits <= 0)
        /// Found sufficiently many files, stop scanning
        m_alive = false;

    firstInvocation = false;
}

bool DirectoryMonitor::isAlive() {
    return m_alive;
}

void DirectoryMonitor::directoryChanged(const QString &path) {
    if (path != m_baseDir)
        qWarning() << "Directory being monitored (" << path << ") is not the same as being base directory: " << m_baseDir;
    if (m_timerWaitForSettledDir.isActive())
        m_timerWaitForSettledDir.stop();
    /// After getting a notification of a change in filesystem,
    /// wait a few seconds to have filesystem being settled
    m_timerWaitForSettledDir.start(5000);
}

void DirectoryMonitor::rescanBaseDirectory() {
    startSearch(m_numExpectedHits);
}
