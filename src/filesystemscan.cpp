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


    Source code written by Thomas Fischer <thomas.fischer@his.se>

 */

#include <QDir>
#include <QDebug>

#include "general.h"
#include "filesystemscan.h"

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

        QStringList files = dir.entryList(m_filters, QDir::Files, QDir::Name & QDir::IgnoreCase);
        foreach(const QString &filename, files) {
            QUrl url = QUrl(dir.absolutePath() + QDir::separator() + filename);
            emit report(QString("<filefinder event=\"hit\" href=\"%1\" />\n").arg(url.toString()));
            emit foundUrl(url);
            ++hits;
            if (hits >= numExpectedHits) break;
        }

        foreach(const QString subdir, dir.entryList(QDir::Dirs)) {
            if (subdir != "." && subdir != "..") {
                queue.append(dir.absolutePath() + QDir::separator() + subdir);
            }
        }
    }

    emit report(QString("<filesystemscan directory=\"%2\" numresults=\"%1\" />\n").arg(QString::number(hits)).arg(DocScan::xmlify(QDir(m_baseDir).absolutePath())));
    m_alive = false;
}

bool FileSystemScan::isAlive()
{
    return m_alive;
}
