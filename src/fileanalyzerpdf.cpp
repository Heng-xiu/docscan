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

#include <poppler/qt4/poppler-qt4.h>

#include <QDebug>

#include "fileanalyzerpdf.h"
#include "watchdog.h"

FileAnalyzerPDF::FileAnalyzerPDF(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
    // nothing
}

bool FileAnalyzerPDF::isAlive()
{
    return false;
}

void FileAnalyzerPDF::analyzeFile(const QString &filename)
{
    Poppler::Document *doc = Poppler::Document::load(filename);

    if (doc != NULL) {
        QString logText = QString("<fileanalysis mimetype=\"application/pdf\" filename=\"%1\">").arg(filename);

        int majorVersion = 0, minorVersion = 0;
        doc->getPdfVersion(&majorVersion, &minorVersion);
        logText += QString("<version major=\"%1\" minor=\"%2\" />\n").arg(QString::number(majorVersion)).arg(QString::number(minorVersion));

        QDateTime creationDate = doc->date("CreationDate").toUTC();
        if (creationDate.isValid())
            logText += QString("<date base=\"creation\" year=\"%1\" month=\"%2\" day=\"%3\">%4</date>\n").arg(creationDate.date().year()).arg(creationDate.date().month()).arg(creationDate.date().day()).arg(creationDate.toString(Qt::ISODate));

        creationDate = doc->date("ModDate").toUTC();
        if (creationDate.isValid())
            logText += QString("<date base=\"modification\" year=\"%1\" month=\"%2\" day=\"%3\">%4</date>\n").arg(creationDate.date().year()).arg(creationDate.date().month()).arg(creationDate.date().day()).arg(creationDate.toString(Qt::ISODate));

        QStringList metaNames = QStringList() << "Title" << "Subject" << "Author" << "Creator" << "Producer" << "Keywords";
        foreach(const QString &metaName, metaNames) {
            if (!doc->info(metaName).isEmpty()) logText += QString("<meta name=\"%1\">%2</meta>\n").arg(metaName.toLower()).arg(doc->info(metaName));
        }

        logText.append("</fileanalysis>\n");

        emit analysisReport(logText);

        delete doc;
    } else
        emit analysisReport(QString("<fileanalysis status=\"error\" filename=\"%1\">\n").arg(filename));
}
