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

#include "fromlogfile.h"

#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QDebug>
#include <QTimer>
#include <QDebug>

#include "general.h"

FromLogFileFileFinder::FromLogFileFileFinder(const QString &logfilename, const QStringList &filters, QObject *parent)
    : FileFinder(parent), m_isAlive(true), filenameRegExp(filters.isEmpty() ? QRegExp() : QRegExp(QString(QStringLiteral("(^|/)(%1)$")).arg(filters.join(QChar('|'))).replace(QChar('.'), QStringLiteral("[.]")).replace(QChar('*'), QStringLiteral(".*"))))
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());

    QFile input(logfilename);
    if (input.open(QFile::ReadOnly)) {
        QTextStream textStream(&input);
        const QString text = textStream.readAll();
        input.close();

        static const QRegExp hitRegExp = QRegExp(QStringLiteral("<filefinder\\b[^>]* event=\"hit\"\\b[^>]* href=\"([^\"]+)\""));

        int p = -1;
        while ((p = hitRegExp.indexIn(text, p + 1)) >= 0) {
            QUrl url = QUrl::fromLocalFile(DocScan::dexmlify(hitRegExp.cap(1)));
            const QString name = url.toString();
            if (filenameRegExp.isEmpty() || filenameRegExp.indexIn(name) >= 0) {
                m_urlSet.insert(url);
            }
        }
        if (m_urlSet.isEmpty())
            qWarning() << "No URLs found in" << logfilename;
    } else
        qWarning() << "Could not find or open old log file" << logfilename;
}

void FromLogFileFileFinder::startSearch(int numExpectedHits)
{
    emit report(objectName(), QString(QStringLiteral("<filefinder count=\"%1\" type=\"fromlogfilefilefinder\" regexp=\"%2\"/>\n")).arg(m_urlSet.count()).arg(DocScan::xmlify(filenameRegExp.pattern())));
    int count = numExpectedHits;
    for (const QUrl &url : const_cast<const QSet<QUrl> &>(m_urlSet)) {
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

FromLogFileDownloader::FromLogFileDownloader(const QString &logfilename, const QStringList &filters, QObject *parent)
    : Downloader(parent), m_logfilename(logfilename), m_isAlive(true), filenameRegExp(filters.isEmpty() ? QRegExp() : QRegExp(QString(QStringLiteral("(^|/)(%1)$")).arg(filters.join(QChar('|'))).replace(QChar('.'), QStringLiteral("[.]")).replace(QChar('*'), QStringLiteral(".*"))))
{
    QTimer::singleShot(500, this, SLOT(startParsingAndEmitting()));
}

void FromLogFileDownloader::startParsingAndEmitting()
{
    QFile input(m_logfilename);
    if (input.open(QFile::ReadOnly)) {
        static const QRegExp hitRegExp = QRegExp(QStringLiteral("<download[^>]* filename=\"([^\"]+)\"[^>]* status=\"success\"[^>]* url=\"([^\"]+)\""));
        static const QRegExp searchEngineNumResultsRegExp = QRegExp(QStringLiteral("<searchengine\\b[^>]* numresults=\"([0-9]*)\""));
        int count = 0;

        QTextStream textStream(&input);
        QString line = textStream.readLine();
        while (!line.isNull()) {
            if (hitRegExp.indexIn(line) >= 0) {
                const QString filename(hitRegExp.cap(1));
                if (filenameRegExp.isEmpty() || filenameRegExp.indexIn(filename) >= 0) {
                    const QUrl url(hitRegExp.cap(2));
                    emit downloaded(url, filename);
                    emit downloaded(filename);
                    ++count;
                }
            } else if (searchEngineNumResultsRegExp.indexIn(line) >= 0)
                emit report(objectName(), QString(QStringLiteral("<searchengine numresults=\"%1\" />")).arg(searchEngineNumResultsRegExp.cap(1)));

            line = textStream.readLine();
        }
        input.close();

        if (count == 0)
            qWarning() << "No filenames found in" << m_logfilename;

        const QString s = QString(QStringLiteral("<downloader count=\"%1\" type=\"fromlogfiledownloader\" regexp=\"%2\"/>\n")).arg(count).arg(DocScan::xmlify(filenameRegExp.pattern()));
        emit report(objectName(), s);
    } else
        qWarning() << "Could not find or open old log file" << m_logfilename;

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
