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

#include <QFileInfo>

#include <rtfreader.h>
#include "TextDocumentRtfOutput.h"

#include "general.h"
#include "fileanalyzerrtf.h"

FileAnalyzerRTF::FileAnalyzerRTF(QObject *parent)
    : FileAnalyzerAbstract(parent), m_isAlive(false)
{
    // TODO
}

bool FileAnalyzerRTF::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerRTF::analyzeFile(const QString &filename)
{
    m_isAlive = true;

    RtfReader::Reader *reader = new RtfReader::Reader;
    bool openSuccess = reader->open(filename);
    if (!openSuccess) {
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"RTF reader could not open file\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
        return;
    }

    QTextDocument doc;
    RtfReader::TextDocumentRtfOutput output(&doc);
    reader->parseTo(&output);
    if (doc.isEmpty()) {
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"RTF reader could not parse file\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
        return;
    }

    QString logText = QString("<fileanalysis status=\"ok\" filename=\"%1\">\n").arg(DocScan::xmlify(filename));
    QString metaText = QLatin1String("<meta>\n");
    QString headerText = QLatin1String("<header>\n");

    const QString mimetype = "application/rtf";
    metaText.append(QString("<fileformat>\n<mimetype>%1</mimetype>\n</fileformat>").arg(mimetype));

    /// file information including size
    QFileInfo fi = QFileInfo(filename);
    metaText.append(QString("<file size=\"%1\" />").arg(fi.size()));

    QDateTime dateTime = output.created();
    if (dateTime.isValid())
        headerText.append(DocScan::formatDate(dateTime.date(), "creation"));
    dateTime = output.revised();
    if (dateTime.isValid())
        headerText.append(DocScan::formatDate(dateTime.date(), "modification"));
    dateTime = output.printed();
    if (dateTime.isValid())
        headerText.append(DocScan::formatDate(dateTime.date(), "print"));

    /// evaluate editor (a.k.a. creator)
    QString text = output.author();
    if (!text.isEmpty())
        headerText.append(QString("<author>%1</author>\n").arg(DocScan::xmlify(text)));

    /// evaluate title
    text = output.title();
    if (!text.isEmpty())
        headerText.append(QString("<title>%1</title>\n").arg(DocScan::xmlify(text)));

    /// evaluate subject
    text = output.subject();
    if (!text.isEmpty())
        headerText.append(QString("<subject>%1</subject>\n").arg(DocScan::xmlify(text)));

    /// close all tags, merge text
    metaText += QLatin1String("</meta>\n");
    logText.append(metaText);
    headerText += QLatin1String("</header>\n");
    logText.append(headerText);
    // logText.append(bodyText);
    logText += QLatin1String("</fileanalysis>\n");

    emit analysisReport(logText);

    reader->close();
    delete reader;

    m_isAlive = false;
}
