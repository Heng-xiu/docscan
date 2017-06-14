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

#include "fakedownloader.h"

#include <QDebug>

FakeDownloader::FakeDownloader(QObject *parent)
    : Downloader(parent), m_counterLocalFiles(0), m_counterErrors(0)
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());
}

void FakeDownloader::download(const QUrl &url)
{
    if (!url.isValid()) {
        qWarning() << "Invalid URL passed to FakeDownloader: " << url.toString();
        const QString logText = QString(QStringLiteral("<download message=\"invalid URL\" status=\"error\" url=\"%1\" />\n")).arg(url.toString());
        emit report(objectName(), logText);
        ++m_counterErrors;
    } else if (!url.isLocalFile()) {
        qWarning() << "Non-local URL passed to FakeDownloader: " << url.toString();
        const QString logText = QString(QStringLiteral("<download message=\"non-local URL\" status=\"error\" url=\"%1\" />\n")).arg(url.toString());
        emit report(objectName(), logText);
        ++m_counterErrors;
    } else {
        const QString localName = url.path();
        qDebug() << "FakeDownloader passing through: " << localName;
        const QString logText = QString(QStringLiteral("<download file=\"%1\" status=\"success\" />\n")).arg(localName);
        emit report(objectName(), logText);

        emit downloaded(localName);
        emit downloaded(url, localName);
        ++m_counterLocalFiles;
    }
}

void FakeDownloader::finalReport()
{
    const QString logText = QString(QStringLiteral("<download count-fail=\"%1\" count-success=\"%2\" />\n")).arg(m_counterErrors).arg(m_counterLocalFiles);
    emit report(objectName(), logText);
}

bool FakeDownloader::isAlive()
{
    return false;
}
