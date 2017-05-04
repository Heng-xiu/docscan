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

#include "filefinderlist.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QUrl>

#include "general.h"

FileFinderList::FileFinderList(const QString &listFile, QObject *parent)
    : FileFinder(parent) {
    m_listFile = listFile;
    m_alive = false;
    qDebug() << "listFile= " << m_listFile;
}

void FileFinderList::startSearch(int numExpectedHits) {
    m_alive = true;

    int hits = 0;
    QFile file(m_listFile);
    if (file.open(QFile::ReadOnly)) {
        QTextStream ts(&file);
        while (!ts.atEnd() && hits < numExpectedHits) {
            const QString filename = ts.readLine();
            QFileInfo fi(filename);
            if (fi.exists() && fi.isFile()) {
                emit report(QString(QStringLiteral("<filefinder event=\"hit\" href=\"%1\" />\n")).arg(DocScan::xmlify(filename)));
                emit foundUrl(QUrl::fromLocalFile(filename));
                ++hits;
            } else
                qWarning() << "File does not exist: " << filename;
        }
        file.close();
    } else
        qWarning() << "Could not open file: " << m_listFile;

    emit report(QString(QStringLiteral("<filefinderlist listfile=\"%2\" numresults=\"%1\" />\n")).arg(QString::number(hits), DocScan::xmlify(m_listFile)));
    m_alive = false;
}

bool FileFinderList::isAlive() {
    return m_alive;
}
