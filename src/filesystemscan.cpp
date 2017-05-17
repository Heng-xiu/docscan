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

#include "filesystemscan.h"

#include <QDir>
#include <QDebug>

#include "general.h"

FileSystemScan::FileSystemScan(const QStringList &filters, const QString &baseDir, QObject *parent)
    : FileFinder(parent), m_filters(filters), m_baseDir(baseDir), m_alive(false)
{
}

void FileSystemScan::startSearch(int numExpectedHits)
{
    m_alive = true;
    QStringList queue = QStringList() << m_baseDir;
    int hits = 0;

    while (hits < numExpectedHits && !queue.isEmpty()) {
        QDir dir = QDir(queue.first());
        queue.removeFirst();

        const QStringList files = dir.entryList(m_filters, QDir::Files, QDir::Name | QDir::IgnoreCase);
        for (const QString &filename : files) {
            QUrl url = QUrl::fromLocalFile(dir.absolutePath() + QDir::separator() + filename);
            emit report(QString(QStringLiteral("<filefinder event=\"hit\" href=\"%1\" />\n")).arg(DocScan::xmlify(url.toString())));
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

    emit report(QString(QStringLiteral("<filesystemscan filter=\"%3\" directory=\"%2\" numresults=\"%1\" />\n")).arg(QString::number(hits), DocScan::xmlify(QDir(m_baseDir).absolutePath()), DocScan::xmlify(m_filters.join(QChar('|')))));
    m_alive = false;
}

bool FileSystemScan::isAlive()
{
    return m_alive;
}
