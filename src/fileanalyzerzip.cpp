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

#include "fileanalyzerzip.h"

#include <QDebug>

#include <quazip.h>
#include <quazipfile.h>

#include "general.h"

FileAnalyzerZIP::FileAnalyzerZIP(QObject *parent)
    : FileAnalyzerAbstract(parent), m_isAlive(false)
{
    FileAnalyzerAbstract::setObjectName(QStringLiteral("fileanalyzerzip"));
}

bool FileAnalyzerZIP::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerZIP::analyzeFile(const QString &filename)
{
    m_isAlive = true;

    QuaZip zipFile(filename);
    if (zipFile.open(QuaZip::mdUnzip)) {
        QString report = QString(QStringLiteral("<fileanalysis filename=\"%1\" status=\"ok\"><embeddedfiles>\n")).arg(DocScan::xmlify(filename));
        for (bool more = zipFile.goToFirstFile(); more; more = zipFile.goToNextFile()) {
            QuaZipFile contentFile(&zipFile, this);
            const QString filename = contentFile.getFileName().isEmpty() ? contentFile.getActualFileName() : contentFile.getFileName();
            if (contentFile.open(QIODevice::ReadOnly)) {
                const QByteArray data = contentFile.readAll();
                contentFile.close();
                const QString mimetype = DocScan::guessMimetype(filename);
                const QString temporaryFilename = dataToTemporaryFile(data, mimetype);
                emit foundEmbeddedFile(temporaryFilename);

                const QString size = contentFile.csize() >= 0 ? QString(QStringLiteral(" size=\"%1\" compressedsize=\"%2\"")).arg(contentFile.usize()).arg(contentFile.csize()) : QString();
                const QString mimetypeAsAttribute = QString(QStringLiteral(" mimetype=\"%1\"")).arg(mimetype);
                const QString embeddedFile = QStringLiteral("<embeddedfile") + size + mimetypeAsAttribute + QStringLiteral("><filename>") + DocScan::xmlify(filename) + QStringLiteral("</filename>") + (temporaryFilename.isEmpty() ? QString() : QStringLiteral("<temporaryfilename>") + temporaryFilename /** no need for DocScan::xmlify */ + QStringLiteral("</temporaryfilename>")) + QStringLiteral("</embeddedfile>\n");
                report.append(embeddedFile);
            } else
                report.append(QString(QStringLiteral("<error status=\"failed-to-open\">%1</error>")).arg(DocScan::xmlify(filename)));
        }
        report.append(QStringLiteral("</embeddedfiles></fileanalysis>\n"));
        emit analysisReport(objectName(), report);
    } else
        emit analysisReport(objectName(), QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" />\n")).arg(DocScan::xmlify(filename)));

    m_isAlive = false;
}
