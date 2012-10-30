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

#include <AbstractRtfOutput.h>
#include <rtfreader.h>
#include <TextDocumentRtfOutput.h>

#include "general.h"
#include "fileanalyzercompoundbinary.h"
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
        emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"RTF reader could not open file\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
        return;
    }

    QTextDocument doc;
    RtfReader::TextDocumentRtfOutput output(&doc);
    reader->parseTo(&output);
    if (doc.isEmpty()) {
        emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"RTF reader could not parse file\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
        return;
    }

    QString logText = QString("<fileanalysis filename=\"%1\" status=\"ok\">\n").arg(DocScan::xmlify(filename));
    QString metaText = QLatin1String("<meta>\n");
    QString headerText = QLatin1String("<header>\n");

    const QString mimetype = "application/rtf";
    metaText.append(QString("<fileformat>\n<mimetype>%1</mimetype>\n</fileformat>").arg(mimetype));

    /// file information including size
    QFileInfo fi = QFileInfo(filename);
    metaText.append(QString("<file size=\"%1\" />").arg(fi.size()));

    /// evaluate dates
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

    /// evaluate comment
    text = output.comment();
    if (!text.isEmpty())
        headerText.append(QString("<comment>%1</comment>\n").arg(DocScan::xmlify(text)));

    /// evaluate and guess editing tool (generator)
    QString guess;
    switch (output.editingTool()) {
    case RtfReader::AbstractRtfOutput::ToolMicrosoftOffice2003orLater:
        guess = guessTool("Microsoft Word 2003 or later");
        break;
    case RtfReader::AbstractRtfOutput::ToolLibreOffice:
        guess = guessTool("LibreOffice");
        break;
    case RtfReader::AbstractRtfOutput::ToolOpenOffice:
        guess = guessTool("OpenOffice");
        break;
    default:
        guess = QString::null;
    }
    if (!guess.isEmpty())
        metaText.append(QString("<tool type=\"generator\">\n%1</tool>\n").arg(guess));

    /// evaluate number of pages
    if (output.numberOfPages() > 0)
        headerText.append(QString("<num-pages origin=\"document\">%1</num-pages>\n").arg(output.numberOfPages()));

    /// evaluate number of words and characters
    int numChars = output.numberOfCharacters();
    //if (numChars<=0 || numChars>=0xfffff) numChars=output.numberOfCharactersWithoutSpaces();
    int numWords = output.numberOfWords();
    QString bodyText = QString("<body length=\"%1\" words=\"%2\" />\n").arg(numChars > 0 && numChars < 0xfffff ? QString::number(numChars) : QLatin1String("unreliable")).arg(numWords > 0 && numWords < 0xfffff ? QString::number(numWords) : QLatin1String("unreliable"));

    /// evaluate language
    headerText.append(QString("<language origin=\"document\">%1</language>\n").arg(FileAnalyzerCompoundBinary::langCodeToISOCode(probeLanguage(filename))));
    const QString plainText = doc.toPlainText();
    /* Disabling aspell, computationally expensive
    if (plainText.length() > 1024)
        headerText.append(QString("<language origin=\"aspell\">%1</language>\n").arg(guessLanguage(plainText)));
     */

    /// evaluate paper size
    if (output.pageHeight() > 0 && output.pageWidth() > 0)
        headerText.append(evaluatePaperSize(output.pageWidth() / 56.694, output.pageHeight() / 56.694));

    // TODO fonts

    /// close all tags, merge text
    metaText += QLatin1String("</meta>\n");
    logText.append(metaText);
    headerText += QLatin1String("</header>\n");
    logText.append(headerText);
    logText.append(bodyText);
    logText += QLatin1String("</fileanalysis>\n");

    emit analysisReport(logText);

    reader->close();
    delete reader;

    m_isAlive = false;
}

int FileAnalyzerRTF::probeLanguage(const QString &filename)
{
    QFile file(filename);
    if (file.open(QFile::ReadOnly)) {
        QByteArray fileData = file.readAll();
        int langCode = 0;
        int p = -1;
        while ((p = fileData.indexOf("\\lang", p + 1)) >= 0) {
            bool ok = false;
            int num = 0;
            if ((num = QString::fromAscii(fileData.mid(p + 5, 4).data()).toInt(&ok)) >= 0 && ok)
                langCode = num;
        }

        file.close();
        return langCode;
    }

    return 0;
}
