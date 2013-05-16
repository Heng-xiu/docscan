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
#include <QDebug>
#include <QDateTime>
#include <QProcess>

#include "popplerwrapper.h"
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
    QString jhoveStandardOutput = QString::null;
    QString jhoveErrorOutput = QString::null;
    if (!m_jhoveShellscript.isEmpty() && !m_jhoveConfigFile.isEmpty()) {
        QProcess jhove(this);
        const QStringList arguments = QStringList() << QLatin1String("-n") << QLatin1String("17") << QLatin1String("/bin/bash") << m_jhoveShellscript << QLatin1String("-c") << m_jhoveConfigFile << QLatin1String("-m") << QLatin1String("PDF-hul") << filename;
        jhove.start(QLatin1String("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        if (jhove.waitForStarted()) {
            jhove.waitForFinished();
            jhoveStandardOutput = QString::fromUtf8(jhove.readAllStandardOutput().data()).replace(QLatin1Char('\n'), QLatin1String("###"));
            if (!jhoveStandardOutput.isEmpty()) {
                jhoveIsPDF = jhoveStandardOutput.contains(QLatin1String("Format: PDF"));
                jhoveWellformedAndValid = jhoveStandardOutput.contains(QLatin1String("Status: Well-Formed and valid"));
                static const QRegExp pdfVersionRegExp(QLatin1String("\\bVersion: ([^#]+)#"));
                jhovePDFversion = pdfVersionRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfVersionRegExp.cap(1) : QString::null;
                static const QRegExp pdfProfileRegExp(QLatin1String("\\bProfile: ([^#]+)(#|$)"));
                jhovePDFprofile = pdfProfileRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfProfileRegExp.cap(1) : QString::null;
            } else {
                jhoveErrorOutput = QString::fromUtf8(jhove.readAllStandardError().data());
                qWarning() << "Output of jhove is empty, stderr is " << jhoveErrorOutput;
            }
        } else
            qWarning() << "Failed to start jhove with as" << arguments.join("_");
    }

    PopplerWrapper *wrapper = PopplerWrapper::createPopplerWrapper(filename);
    if (wrapper != NULL) {
        QString guess;

        QString logText;
        QString metaText = QString::null;
        QString headerText = QString::null;
        QString bodyText = QString::null;

        /// file format including mime type and file format version
        int majorVersion = 0, minorVersion = 0;
        wrapper->getPdfVersion(majorVersion, minorVersion);
        metaText.append(QString("<fileformat>\n<mimetype>application/pdf</mimetype>\n<version major=\"%1\" minor=\"%2\">%1.%2</version>\n<security locked=\"%3\" encrypted=\"%4\" />\n</fileformat>\n").arg(majorVersion).arg(minorVersion).arg(wrapper->isLocked() ? QLatin1String("yes") : QLatin1String("no")).arg(wrapper->isEncrypted() ? QLatin1String("yes") : QLatin1String("no")));

        if (jhoveIsPDF) {
            /// insert data from jHove
            metaText.append(QString(QLatin1String("<jhove wellformed=\"%1\"")).arg(jhoveWellformedAndValid ? QLatin1String("yes") : QLatin1String("no")));
            if (jhovePDFversion.isEmpty() && jhovePDFprofile.isEmpty() && jhoveErrorOutput.isEmpty())
                metaText.append(QLatin1String(" />\n"));
            else {
                metaText.append(QLatin1String(">\n"));
                if (!jhovePDFversion.isEmpty())
                    metaText.append(QString(QLatin1String("<version>%1</version>\n")).arg(DocScan::xmlify(jhovePDFversion)));
                if (!jhovePDFprofile.isEmpty())
                    metaText.append(QString(QLatin1String("<profile linear=\"%2\" tagged=\"%3\" pdfa1a=\"%4\" pdfa1b=\"%5\" pdfx3=\"%6\">%1</profile>\n")).arg(DocScan::xmlify(jhovePDFprofile)).arg(jhovePDFprofile.contains(QLatin1String("Linearized PDF")) ? QLatin1String("yes") : QLatin1String("no")).arg(jhovePDFprofile.contains(QLatin1String("Tagged PDF")) ? QLatin1String("yes") : QLatin1String("no")).arg(jhovePDFprofile.contains(QLatin1String("ISO PDF/A-1, Level A")) ? QLatin1String("yes") : QLatin1String("no")).arg(jhovePDFprofile.contains(QLatin1String("ISO PDF/A-1, Level B")) ? QLatin1String("yes") : QLatin1String("no")).arg(jhovePDFprofile.contains(QLatin1String("ISO PDF/X-3")) ? QLatin1String("yes") : QLatin1String("no")));
                if (!jhoveStandardOutput.isEmpty())
                    metaText.append(QString(QLatin1String("<output>%1</output>\n")).arg(DocScan::xmlify(jhoveStandardOutput.replace(QLatin1String("###"), QLatin1String("\n")))));
                if (!jhoveErrorOutput.isEmpty())
                    metaText.append(QString(QLatin1String("<error>%1</error>\n")).arg(DocScan::xmlify(jhoveErrorOutput)));
                metaText.append(QLatin1String("</jhove>\n"));
            }
        }

        /// file information including size
        QFileInfo fi = QFileInfo(filename);
        metaText.append(QString("<file size=\"%1\" />\n").arg(fi.size()));

        /// guess and evaluate editor (a.k.a. creator)
        QString creator = wrapper->info("Creator");
        guess.clear();
        if (!creator.isEmpty())
            guess = guessTool(creator, wrapper->info("Title"));
        if (!guess.isEmpty())
            metaText.append(QString("<tool type=\"editor\">\n%1</tool>\n").arg(guess));
        /// guess and evaluate producer
        QString producer = wrapper->info("Producer");
        guess.clear();
        if (!producer.isEmpty())
            guess = guessTool(producer, wrapper->info("Title"));
        if (!guess.isEmpty())
            metaText.append(QString("<tool type=\"producer\">\n%1</tool>\n").arg(guess));

        if (!wrapper->isLocked()) {
            /// some functions are sensitive if PDF is locked

            /// retrieve font information
            const QStringList fontNames = wrapper->fontNames();
            static QRegExp fontNameNormalizer("^[A-Z]+\\+", Qt::CaseInsensitive);
            QSet<QString> knownFonts;
            foreach(const QString &fi, fontNames) {
                QStringList fields = fi.split(QLatin1Char('|'), QString::KeepEmptyParts);
                if (fields.length() != 2) continue;
                const QString fontName = fields[0].replace(fontNameNormalizer, "");
                if (fontName.isEmpty()) continue;
                if (knownFonts.contains(fontName)) continue; else knownFonts.insert(fontName);
                metaText.append(QString("<font>\n%1</font>").arg(guessFont(fontName, fields[1])));
            }

        }

        /// format creation date
        QDate date = wrapper->date("CreationDate").toUTC().date();
        if (date.isValid())
            headerText.append(formatDate(date, creationDate));
        /// format modification date
        date = wrapper->date("ModDate").toUTC().date();
        if (date.isValid())
            headerText.append(formatDate(date, modificationDate));

        /// retrieve author
        QString author = wrapper->info("Author").simplified();
        if (!author.isEmpty())
            headerText.append(QString("<author>%1</author>\n").arg(DocScan::xmlify(author)));

        /// retrieve title
        QString title = wrapper->info("Title").simplified();
        /// clean-up title
        if (microsoftToolRegExp.indexIn(title) == 0)
            title = microsoftToolRegExp.cap(3);
        if (!title.isEmpty())
            headerText.append(QString("<title>%1</title>\n").arg(DocScan::xmlify(title)));

        /// retrieve subject
        QString subject = wrapper->info("Subject").simplified();
        if (!subject.isEmpty())
            headerText.append(QString("<subject>%1</subject>\n").arg(DocScan::xmlify(subject)));

        /// retrieve keywords
        QString keywords = wrapper->info("Keywords").simplified();
        if (!keywords.isEmpty())
            headerText.append(QString("<keyword>%1</keyword>\n").arg(DocScan::xmlify(keywords)));

        if (!wrapper->isLocked()) {
            /// some functions are sensitive if PDF is locked

            /// guess language using aspell
            const QString text = wrapper->plainText();
            /* Disabling aspell, computationally expensive
            QString language = guessLanguage(text);
            if (!language.isEmpty())
                headerText.append(QString("<language origin=\"aspell\">%1</language>\n").arg(language));
             */
            bodyText = QString("<body length=\"%1\">\n").arg(text.length());
            bodyText = bodyText.append(wrapper->imagesLog()).append(QLatin1String("</body>\n"));

            /// look into first page for info
            int numPages = wrapper->numPages();
            headerText.append(QString("<num-pages>%1</num-pages>\n").arg(numPages));
            if (numPages > 0) {
                /// retrieve and evaluate paper size
                QSizeF size = wrapper->pageSize();
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

        delete wrapper;
    } else
        emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" />\n").arg(filename));

    m_isAlive = false;
}
