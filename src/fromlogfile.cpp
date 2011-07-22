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

#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QDebug>
#include <QTimer>

#include "fromlogfile.h"

FromLogFileFileFinder::FromLogFileFileFinder(const QString &logfilename, QObject *parent)
    : FileFinder(parent), m_isAlive(true)
{
    QFile input(logfilename);
    if (input.open(QFile::ReadOnly)) {
        QTextStream textStream(&input);
        const QString text = textStream.readAll();
        input.close();

        QRegExp hitRegExp = QRegExp(QLatin1String("<filefinder event=\"hit\" href=\"([^\"]+)\"/>"));
        int p = -1;
        while ((p = hitRegExp.indexIn(text, p + 1)) >= 0) {
            qDebug() << "FromLogFileFileFinder  url=" << hitRegExp.cap(1);
            m_urlSet.insert(QUrl(hitRegExp.cap(1)));
        }
    }
}

void FromLogFileFileFinder::startSearch(int numExpectedHits)
{
    emit report(QString("<filefinder type=\"fromlogfilefilefinder\" count=\"%1\"/>\n").arg(m_urlSet.count()));
    int count = numExpectedHits;
    foreach(const QUrl &url, m_urlSet) {
        if (count <= 0) break;
        emit foundUrl(url);
        --count;
    }
    m_isAlive = false;
}

bool FromLogFileFileFinder::isAlive()
{
    return m_isAlive;
}

FromLogFileDownloader::FromLogFileDownloader(const QString &logfilename, QObject *parent)
    : Downloader(parent), m_isAlive(true)
{
    QFile input(logfilename);
    if (input.open(QFile::ReadOnly)) {
        QTextStream textStream(&input);
        const QString text = textStream.readAll();
        input.close();

        QRegExp hitRegExp = QRegExp(QLatin1String("<download url=\"([^\"]+)\" filename=\"([^\"]+)\" status=\"success\"/>"));
        int p = -1;
        while ((p = hitRegExp.indexIn(text, p + 1)) >= 0) {
            qDebug() << "FromLogFileDownloader  url=" << hitRegExp.cap(1) << "  filename=" << hitRegExp.cap(2);
            m_fileSet.insert(QPair<QString, QUrl>(hitRegExp.cap(2), QUrl(hitRegExp.cap(1))));
        }
    }

    QTimer::singleShot(500, this, SLOT(startEmitting()));
}

void FromLogFileDownloader::startEmitting()
{
    const QString s = QString("<downloader type=\"fromlogfiledownloader\" count=\"%1\"/>\n").arg(m_fileSet.count());
    emit report(s);
    for (QSet<QPair<QString, QUrl> >::ConstIterator it = m_fileSet.constBegin(); it != m_fileSet.constEnd(); ++it) {
        emit downloaded(it->second, it->first);
        emit downloaded(it->first);
    }
    m_isAlive = false;
}

void FromLogFileDownloader::download(const QUrl &url)
{
    qWarning() << "This should never be called (url =" << url.toString() << ")";
}

void FromLogFileDownloader::finalReport()
{
    // nothing
}

bool FromLogFileDownloader::isAlive()
{
    return m_isAlive;
}
