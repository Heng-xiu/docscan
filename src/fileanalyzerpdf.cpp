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
    Poppler::Document *doc = Poppler::Document::load(filename);

    if (doc != NULL) {
        QString logText = QString("<fileanalysis mimetype=\"application/pdf\" filename=\"%1\">").arg(DocScan::xmlify(filename));

        int majorVersion = 0, minorVersion = 0;
        doc->getPdfVersion(&majorVersion, &minorVersion);
        logText += QString("<fileformat-version major=\"%1\" minor=\"%2\">%1.%2</fileformat-version>\n").arg(QString::number(majorVersion)).arg(QString::number(minorVersion));

        QDate date = doc->date("CreationDate").toUTC().date();
        if (date.isValid())
            logText += DocScan::formatDate(date, "creation");

        date = doc->date("ModDate").toUTC().date();
        if (date.isValid())
            logText += DocScan::formatDate(date, "modification");

        QStringList metaNames = QStringList() << "Author" << "Keywords";
        foreach(const QString &metaName, metaNames) {
            QString text = doc->info(metaName);
            if (!text.isEmpty()) logText += QString("<meta name=\"%1\">%2</meta>\n").arg(metaName.toLower()).arg(DocScan::xmlify(text));
        }

        metaNames = QStringList() << "Title" << "Subject";
        foreach(const QString &metaName, metaNames) {
            QString text = doc->info(metaName);
            if (!text.isEmpty()) logText += QString("<%1>%2</%1>\n").arg(metaName.toLower()).arg(DocScan::xmlify(text));
        }

        QString text = doc->info("Creator");
        QString title = doc->info("Title");
        if (title.startsWith("Microsoft Word - "))
            logText += QString("<generator license=\"proprietary\" type=\"editor\">Microsoft Word</generator>\n");
        else if (!text.isEmpty())
            logText += QString("<generator license=\"%2\" type=\"editor\">%1</generator>\n").arg(DocScan::xmlify(text)).arg(guessLicenseFromProduct(text));
        text = doc->info("Producer");
        if (!text.isEmpty()) {
            QString arguments;
            if (text.indexOf("Quartz PDFContext") >= 0 || text.indexOf("Mac OS X") >= 0 || text.indexOf("Macintosh") >= 0)
                arguments += " opsys=\"unix|macos\"";
            else if (text.indexOf("Windows") >= 0 || text.indexOf(".dll") >= 0 || text.indexOf("PDF Complete") >= 0 || text.indexOf("Nitro PDF") >= 0 || text.indexOf("PrimoPDF") >= 0)
                arguments += " opsys=\"windows\"";

            logText += QString("<generator%2 license=\"%3\" type=\"postprocessing\">%1</generator>\n").arg(DocScan::xmlify(text)).arg(arguments).arg(guessLicenseFromProduct(text));
        }

        logText += "<meta name=\"language\" origin=\"aspell\">" + guessLanguage(plainText(doc)) + "</meta>\n";

        QString documentProperties;
        if (doc->numPages() > 0) {
            Poppler::Page *page = doc->page(0);
            QSize size = page->pageSize();
            if (size.width() > 0 && size.height() > 0) {
                documentProperties += QString("<pagesize width=\"%1\" height=\"%2\" unit=\"1/72inch\" />\n").arg(size.width()).arg(size.height());
                int mmw = size.width() * 0.3527778;
                int mmh = size.height() * 0.3527778;
                documentProperties += evaluatePaperSize(mmw, mmh);
            }

            logText += "<statistics type=\"pagecount\" origin=\"document\">" + QString::number(doc->numPages()) + "</statistics>\n";
        }

        QList<Poppler::FontInfo> fontList = doc->fonts();
        if (!fontList.isEmpty()) {
            logText += QString("<statistics type=\"fonts\" origin=\"document\" count=\"%1\">\n").arg(fontList.count());
            static QRegExp fontNameNormalizer("^[A-Z]+\\+", Qt::CaseInsensitive);
            foreach(Poppler::FontInfo fi, fontList) {
                QString fontName = fi.name().replace(fontNameNormalizer, "");
                logText += QString("<font name=\"%1\" type=\"%2\" />\n").arg(DocScan::xmlify(fontName)).arg(DocScan::xmlify(fi.typeName()));
            }
            logText += "</statistics>\n";
        }
        logText += "<statistics type=\"size\" unit=\"bytes\">" + QString::number(QFileInfo(filename).size()) + "</statistics>\n";
        logText += documentProperties;

        logText.append("</fileanalysis>\n");

        emit analysisReport(logText);

        delete doc;
    } else
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-fileformat\" filename=\"%1\" />\n").arg(filename));

    m_isAlive = false;
}

QString FileAnalyzerPDF::plainText(Poppler::Document *doc)
{
    QString result;

    for (int i = 0; i < doc->numPages() && result.length() < 16384; ++i)
        result += doc->page(i)->text(QRect());

    return result;
}
