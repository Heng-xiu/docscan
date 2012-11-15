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

#include <QFileInfo>
#include <QDebug>
#include <QDateTime>

#include "fileanalyzerpdf.h"
#include "watchdog.h"
#include "general.h"

FileAnalyzerPDF::FileAnalyzerPDF(QObject *parent)
    : FileAnalyzerAbstract(parent), m_isAlive(false)
{
    // nothing
}

bool FileAnalyzerPDF::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerPDF::analyzeFile(const QString &filename)
{
    m_isAlive = true;
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    Poppler::Document *doc = Poppler::Document::load(filename);

    if (doc != NULL) {
        QString guess;

        QString logText;
        QString metaText = QLatin1String("<meta>\n");
        QString headerText = QLatin1String("<header>\n");

        /// file format including mime type and file format version
        int majorVersion = 0, minorVersion = 0;
        doc->getPdfVersion(&majorVersion, &minorVersion);
        metaText.append(QString("<fileformat>\n<mimetype>application/pdf</mimetype>\n<version major=\"%1\" minor=\"%2\">%1.%2</version>\n</fileformat>\n").arg(majorVersion).arg(minorVersion));

        /// file information including size
        QFileInfo fi = QFileInfo(filename);
        metaText.append(QString("<file size=\"%1\" />").arg(fi.size()));

        /// guess and evaluate editor (a.k.a. creator)
        QString creator = doc->info("Creator");
        guess.clear();
        if (!creator.isEmpty())
            guess = guessTool(creator, doc->info("Title"));
        if (!guess.isEmpty())
            metaText.append(QString("<tool type=\"editor\">\n%1</tool>\n").arg(guess));
        /// guess and evaluate producer
        QString producer = doc->info("Producer");
        guess.clear();
        if (!producer.isEmpty())
            guess = guessTool(producer, doc->info("Title"));
        if (!guess.isEmpty())
            metaText.append(QString("<tool type=\"producer\">\n%1</tool>\n").arg(guess));

        /// retrieve font information

        QList<Poppler::FontInfo> fontList = doc->fonts();
        static QRegExp fontNameNormalizer("^[A-Z]+\\+", Qt::CaseInsensitive);
        QSet<QString> knownFonts;
        foreach(Poppler::FontInfo fi, fontList) {
            const QString fontName = fi.name().replace(fontNameNormalizer, "");
            if (fontName.isEmpty()) continue;
            if (knownFonts.contains(fontName)) continue; else knownFonts.insert(fontName);
            metaText.append(QString("<font>\n%1</font>").arg(guessFont(fontName, fi.typeName())));
        }

        /// format creation date
        QDate date = doc->date("CreationDate").toUTC().date();
        if (date.isValid())
            headerText.append(formatDate(date, creationDate));
        /// format modification date
        date = doc->date("ModDate").toUTC().date();
        if (date.isValid())
            headerText.append(formatDate(date, modificationDate));

        /// retrieve author
        QString author = doc->info("Author").simplified();
        if (!author.isEmpty())
            headerText.append(QString("<author>%1</author>\n").arg(DocScan::xmlify(author)));

        /// retrieve title
        QString title = doc->info("Title").simplified();
        /// clean-up title
        if (microsoftToolRegExp.indexIn(title) == 0)
            title = microsoftToolRegExp.cap(3);
        if (!title.isEmpty())
            headerText.append(QString("<title>%1</title>\n").arg(DocScan::xmlify(title)));

        /// retrieve subject
        QString subject = doc->info("Subject").simplified();
        if (!subject.isEmpty())
            headerText.append(QString("<subject>%1</subject>\n").arg(DocScan::xmlify(subject)));

        /// retrieve keywords
        QString keywords = doc->info("Keywords").simplified();
        if (!keywords.isEmpty())
            headerText.append(QString("<keyword>%1</keyword>\n").arg(DocScan::xmlify(keywords)));

        /// guess language using aspell
        const QString text = plainText(doc);
        /* Disabling aspell, computationally expensive
        QString language = guessLanguage(text);
        if (!language.isEmpty())
            headerText.append(QString("<language origin=\"aspell\">%1</language>\n").arg(language));
         */

        /// look into first page for info
        int numPages = doc->numPages();
        headerText.append(QString("<num-pages>%1</num-pages>\n").arg(numPages));
        if (numPages > 0) {
            Poppler::Page *page = doc->page(0);
            /// retrieve and evaluate paper size
            QSize size = page->pageSize();
            int mmw = size.width() * 0.3527778;
            int mmh = size.height() * 0.3527778;
            if (mmw > 0 && mmh > 0) {
                headerText += evaluatePaperSize(mmw, mmh);
            }
        }

        QString bodyText = QString("<body length=\"%1\" />\n").arg(text.length());

        /// close all tags, merge text
        metaText += QLatin1String("</meta>\n");
        logText.append(metaText);
        headerText += QLatin1String("</header>\n");
        logText.append(headerText);
        logText.append(bodyText);
        logText += QLatin1String("</fileanalysis>\n");

        qint64 endTime = QDateTime::currentMSecsSinceEpoch();
        logText.prepend(QString("<fileanalysis filename=\"%1\" status=\"ok\" time=\"%2\">\n").arg(DocScan::xmlify(filename)).arg(endTime - startTime));

        emit analysisReport(logText);

        delete doc;
    } else
        emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" />\n").arg(filename));

    m_isAlive = false;
}

QString FileAnalyzerPDF::plainText(Poppler::Document *doc)
{
    QString result;

    for (int i = 0; i < doc->numPages() && result.length() < 16384; ++i)
        result += doc->page(i)->text(QRect());

    return result;
}
