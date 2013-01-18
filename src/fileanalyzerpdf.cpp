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
#include <QProcess>

#include "fileanalyzerpdf.h"
#include "watchdog.h"
#include "general.h"

FileAnalyzerPDF::FileAnalyzerPDF(QObject *parent)
    : FileAnalyzerAbstract(parent), m_isAlive(false), m_jhoveShellscript(QString::null), m_jhoveConfigFile(QString::null)
{
    // nothing
}

bool FileAnalyzerPDF::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerPDF::setupJhove(const QString &shellscript, const QString &configFile)
{
    m_jhoveShellscript = shellscript;
    m_jhoveConfigFile = configFile;
}

void FileAnalyzerPDF::analyzeFile(const QString &filename)
{
    m_isAlive = true;
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    bool jhoveIsPDF = false;
    bool jhoveWellformedAndValid = false;
    QString jhovePDFversion = QString::null;
    QString jhovePDFprofile = QString::null;
    if (!m_jhoveShellscript.isEmpty() && !m_jhoveConfigFile.isEmpty()) {
        QProcess jhove(this);
        const QStringList arguments = QStringList() << m_jhoveShellscript << QLatin1String("-c") << m_jhoveConfigFile << QLatin1String("-m") << QLatin1String("PDF-hul") << filename;
        jhove.start(QLatin1String("/bin/bash"), arguments, QIODevice::ReadOnly);
        if (jhove.waitForStarted()) {
            jhove.waitForFinished();
            const QString jhoveOutput = QString::fromUtf8(jhove.readAllStandardOutput().data()).replace(QLatin1Char('\n'), QLatin1Char('#'));
            if (!jhoveOutput.isEmpty()) {
                jhoveIsPDF = jhoveOutput.contains(QLatin1String("Format: PDF"));
                jhoveWellformedAndValid = jhoveOutput.contains(QLatin1String("Status: Well-Formed and valid"));
                static const QRegExp pdfVersionRegExp(QLatin1String("\\bVersion: ([^#]+)#"));
                jhovePDFversion = pdfVersionRegExp.indexIn(jhoveOutput) >= 0 ? pdfVersionRegExp.cap(1) : QString::null;
                static const QRegExp pdfProfileRegExp(QLatin1String("\\bProfile: ([^#]+)#"));
                jhovePDFprofile = pdfProfileRegExp.indexIn(jhoveOutput) >= 0 ? pdfProfileRegExp.cap(1) : QString::null;
            } else
                qWarning() << "Output of jhove is empty, stderr is " << QString::fromUtf8(jhove.readAllStandardError().data());
        } else
            qWarning() << "Failed to start jhove with as" << arguments.join("_");
    }

    Poppler::Document *doc = Poppler::Document::load(filename);
    if (doc != NULL) {
        QString guess;

        QString logText;
        QString metaText = QString::null;
        QString headerText = QString::null;
        QString bodyText = QString::null;

        /// file format including mime type and file format version
        int majorVersion = 0, minorVersion = 0;
        doc->getPdfVersion(&majorVersion, &minorVersion);
        metaText.append(QString("<fileformat>\n<mimetype>application/pdf</mimetype>\n<version major=\"%1\" minor=\"%2\">%1.%2</version>\n<security locked=\"%3\" encrypted=\"%4\" />\n</fileformat>\n").arg(majorVersion).arg(minorVersion).arg(doc->isLocked() ? QLatin1String("yes") : QLatin1String("no")).arg(doc->isEncrypted() ? QLatin1String("yes") : QLatin1String("no")));

        if (jhoveIsPDF) {
            /// insert data from jHove
            metaText.append(QString(QLatin1String("<jhove wellformed=\"%1\"")).arg(jhoveWellformedAndValid ? QLatin1String("yes") : QLatin1String("no")));
            if (jhovePDFversion.isEmpty() && jhovePDFprofile.isEmpty())
                metaText.append(QLatin1String(" />\n"));
            else {
                metaText.append(QLatin1String(">\n"));
                if (!jhovePDFversion.isEmpty())
                    metaText.append(QString(QLatin1String("<version>%1</version>\n")).arg(DocScan::xmlify(jhovePDFversion)));
                if (!jhovePDFprofile.isEmpty())
                    metaText.append(QString(QLatin1String("<profile linear=\"%2\" tagged=\"%3\" pdfa1a=\"%4\" pdfa1b=\"%5\">%1</profile>\n")).arg(DocScan::xmlify(jhovePDFprofile)).arg(jhovePDFprofile.contains(QLatin1String("Linearized PDF")) ? QLatin1String("yes") : QLatin1String("no")).arg(jhovePDFprofile.contains(QLatin1String("Tagged PDF")) ? QLatin1String("yes") : QLatin1String("no")).arg(jhovePDFprofile.contains(QLatin1String("ISO PDF/A-1, Level A")) ? QLatin1String("yes") : QLatin1String("no")).arg(jhovePDFprofile.contains(QLatin1String("ISO PDF/A-1, Level B")) ? QLatin1String("yes") : QLatin1String("no")));
                metaText.append(QLatin1String("</jhove>\n"));
            }
        }

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

        if (!doc->isLocked() && !doc->isEncrypted()) {
            /// some functions are sensitive if PDF is locked

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

        if (!doc->isLocked() && !doc->isEncrypted()) {
            /// some functions are sensitive if PDF is locked or encrypted

            /// guess language using aspell
            const QString text = plainText(doc);
            /* Disabling aspell, computationally expensive
            QString language = guessLanguage(text);
            if (!language.isEmpty())
                headerText.append(QString("<language origin=\"aspell\">%1</language>\n").arg(language));
             */
            bodyText = QString("<body length=\"%1\" />\n").arg(text.length());

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

        }

        /// close all tags, merge text
        if (!metaText.isEmpty())
            logText.append(QLatin1String("<meta>\n")).append(metaText).append(QLatin1String("</meta>\n"));
        if (!headerText.isEmpty())
            logText.append(QLatin1String("<header>\n")).append(headerText).append(QLatin1String("</header>\n"));
        if (!bodyText.isEmpty())
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
