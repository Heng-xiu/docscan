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
#include "guessing.h"
#include "general.h"

FileAnalyzerPDF::FileAnalyzerPDF(QObject *parent)
    : FileAnalyzerAbstract(parent), m_isAlive(false), m_jhoveShellscript(QString::null), m_jhoveConfigFile(QString::null), m_jhoveVerbose(false)
{
    // nothing
}

bool FileAnalyzerPDF::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerPDF::setupJhove(const QString &shellscript, const QString &configFile, bool verbose)
{
    m_jhoveShellscript = shellscript;
    m_jhoveConfigFile = configFile;
    m_jhoveVerbose = verbose;
}

void FileAnalyzerPDF::setupVeraPDF(const QString &cliTool)
{
    m_veraPDFcliTool = cliTool;
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
    int jhoveExitCode = -1;
    if (!m_jhoveShellscript.isEmpty() && !m_jhoveConfigFile.isEmpty()) {
        QProcess jhove(this);
        const QStringList arguments = QStringList() << QStringLiteral("-n") << QStringLiteral("17") << QStringLiteral("/bin/bash") << m_jhoveShellscript << QStringLiteral("-c") << m_jhoveConfigFile << QStringLiteral("-m") << QStringLiteral("PDF-hul") << filename;
        jhove.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        if (jhove.waitForStarted()) {
            jhove.waitForFinished();
            jhoveExitCode = jhove.exitCode();
            jhoveStandardOutput = QString::fromUtf8(jhove.readAllStandardOutput().data()).replace(QLatin1Char('\n'), QStringLiteral("###"));
            jhoveErrorOutput = QString::fromUtf8(jhove.readAllStandardError().data()).replace(QLatin1Char('\n'), QStringLiteral("###"));
            if (!jhoveStandardOutput.isEmpty()) {
                jhoveIsPDF = jhoveStandardOutput.contains(QStringLiteral("Format: PDF"));
                jhoveWellformedAndValid = jhoveStandardOutput.contains(QStringLiteral("Status: Well-Formed and valid"));
                static const QRegExp pdfVersionRegExp(QStringLiteral("\\bVersion: ([^#]+)#"));
                jhovePDFversion = pdfVersionRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfVersionRegExp.cap(1) : QString::null;
                static const QRegExp pdfProfileRegExp(QStringLiteral("\\bProfile: ([^#]+)(#|$)"));
                jhovePDFprofile = pdfProfileRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfProfileRegExp.cap(1) : QString::null;
            } else {
                qWarning() << "Output of jhove is empty, stderr is " << jhoveErrorOutput;
            }
        } else
            qWarning() << "Failed to start jhove with as" << arguments.join("_");
    }

    bool veraPDFIsPDF = false;
    bool veraPDFIsPDFA1B = false;
    QString veraPDFStandardOutput = QString::null;
    QString veraPDFErrorOutput = QString::null;
    long veraPDFfilesize = 0;
    int veraPDFExitCode = -1;
    if (jhoveIsPDF && !m_veraPDFcliTool.isEmpty()) {
        QProcess veraPDF(this);
        const QStringList arguments = QStringList() << QStringList() << QStringLiteral("-n") << QStringLiteral("17") << QStringLiteral("ionice") << QStringLiteral("-c") << QStringLiteral("3") << m_veraPDFcliTool << QStringLiteral("-f") << QStringLiteral("1b") << QStringLiteral("--maxfailures") << QStringLiteral("1") << QStringLiteral("--format") << QStringLiteral("xml") << filename;
        veraPDF.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        if (veraPDF.waitForStarted()) {
            veraPDF.waitForFinished();
            veraPDFExitCode = veraPDF.exitCode();
            veraPDFStandardOutput = QString::fromUtf8(veraPDF.readAllStandardOutput().data());
            veraPDFErrorOutput = QString::fromUtf8(veraPDF.readAllStandardError().data());
            if (veraPDFExitCode == 0 && !veraPDFStandardOutput.isEmpty()) {
                const QString startOfOutput = veraPDFStandardOutput.left(2048);
                veraPDFIsPDF = startOfOutput.contains(QStringLiteral(" flavour=\"PDF"));
                veraPDFIsPDFA1B = startOfOutput.contains(QStringLiteral(" flavour=\"PDFA_1_B\"")) && startOfOutput.contains(QStringLiteral(" isCompliant=\"true\""));
                const int p1 = startOfOutput.indexOf(QStringLiteral("itemDetails size=\""));
                if (p1 > 1) {
                    const int p2 = startOfOutput.indexOf(QStringLiteral("\""), p1 + 18);
                    if (p2 > p1) {
                        bool ok = false;
                        veraPDFfilesize = startOfOutput.mid(p1 + 18, p2 - p1 - 18).toLong(&ok);
                        if (!ok) veraPDFfilesize = 0;
                    }
                }
            } else
                qWarning() << "Execution of veraPDF failed: " << veraPDFErrorOutput;
        } else
            qWarning() << "Failed to start veraPDF with arguments " << arguments.join("_");
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
        metaText.append(QString("<fileformat>\n<mimetype>application/pdf</mimetype>\n<version major=\"%1\" minor=\"%2\">%1.%2</version>\n<security locked=\"%3\" encrypted=\"%4\" />\n</fileformat>\n").arg(majorVersion).arg(minorVersion).arg(wrapper->isLocked() ? QStringLiteral("yes") : QStringLiteral("no")).arg(wrapper->isEncrypted() ? QStringLiteral("yes") : QStringLiteral("no")));

        if (jhoveIsPDF) {
            /// insert data from jHove
            metaText.append(QString(QStringLiteral("<jhove exitcode=\"%2\" wellformed=\"%1\"")).arg(jhoveWellformedAndValid ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhoveExitCode));
            if (jhovePDFversion.isEmpty() && jhovePDFprofile.isEmpty() && jhoveStandardOutput.isEmpty() && jhoveErrorOutput.isEmpty())
                metaText.append(QStringLiteral(" />\n"));
            else {
                metaText.append(QStringLiteral(">\n"));
                if (!jhovePDFversion.isEmpty())
                    metaText.append(QString(QStringLiteral("<version>%1</version>\n")).arg(DocScan::xmlify(jhovePDFversion)));
                if (!jhovePDFprofile.isEmpty())
                    metaText.append(QString(QStringLiteral("<profile linear=\"%2\" tagged=\"%3\" pdfa1a=\"%4\" pdfa1b=\"%5\" pdfx3=\"%6\">%1</profile>\n")).arg(DocScan::xmlify(jhovePDFprofile)).arg(jhovePDFprofile.contains(QStringLiteral("Linearized PDF")) ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhovePDFprofile.contains(QStringLiteral("Tagged PDF")) ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhovePDFprofile.contains(QStringLiteral("ISO PDF/A-1, Level A")) ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhovePDFprofile.contains(QStringLiteral("ISO PDF/A-1, Level B")) ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhovePDFprofile.contains(QStringLiteral("ISO PDF/X-3")) ? QStringLiteral("yes") : QStringLiteral("no")));
                if (m_jhoveVerbose && !jhoveStandardOutput.isEmpty())
                    metaText.append(QString(QStringLiteral("<output>%1</output>\n")).arg(DocScan::xmlify(jhoveStandardOutput.replace(QStringLiteral("###"), QStringLiteral("\n")))));
                if (!jhoveErrorOutput.isEmpty())
                    metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(jhoveErrorOutput.replace(QStringLiteral("###"), QStringLiteral("\n")))));
                metaText.append(QStringLiteral("</jhove>\n"));
            }
        }

        if (!veraPDFStandardOutput.isEmpty()) {
            /// insert XML data from veraPDF
            const int p = veraPDFStandardOutput.indexOf(QStringLiteral("?>"));
            if (p > 0) {
                metaText.append(QString(QStringLiteral("<verapdf exitcode=\"%1\" pdf=\"%2\" pdfa1b=\"%3\" filesize=\"%4\">\n")).arg(veraPDFExitCode).arg(veraPDFIsPDF ? QStringLiteral("true") : QStringLiteral("false")).arg(veraPDFIsPDFA1B ? QStringLiteral("true") : QStringLiteral("false")).arg(veraPDFfilesize));
                metaText.append(veraPDFStandardOutput.mid(p + 3));
                metaText.append(QStringLiteral("</verapdf>\n"));
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
                if (fields.length() < 2) continue;
                const QString fontName = fields[0].replace(fontNameNormalizer, "");
                QString fontFilename;
                const int p1 = fi.indexOf(QStringLiteral("|FONTFILENAME:"));
                if (p1 > 0) {
                    const int p2 = fi.indexOf(QStringLiteral("|"), p1 + 4);
                    fontFilename = fi.mid(p1 + 15, p2 - p1 - 15);
                }
                if (fontName.isEmpty()) continue;
                if (knownFonts.contains(fontName)) continue; else knownFonts.insert(fontName);
                metaText.append(QString(QStringLiteral("<font embedded=\"%2\" subset=\"%3\"%4>\n%1</font>\n")).arg(Guessing::fontToXML(fontName, fields[1])).arg(fi.contains(QStringLiteral("|EMBEDDED:1")) ? QStringLiteral("yes") : QStringLiteral("no")).arg(fi.contains(QStringLiteral("|SUBSET:1")) ? QStringLiteral("yes") : QStringLiteral("no")).arg(fontFilename.isEmpty() ? QString() : QString(" filename=\"%1\"").arg(fontFilename)));
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

            if (textExtraction > teNone) {
                int length = 0;
                const QString text = wrapper->plainText(&length);
                QString language;
                if (textExtraction >= teAspell) {
                    language = guessLanguage(text);
                    if (!language.isEmpty())
                        headerText.append(QString("<language origin=\"aspell\">%1</language>\n").arg(language));
                }
                bodyText = QString(QStringLiteral("<body length=\"%1\"")).arg(length);
                if (textExtraction >= teFullText)
                    bodyText.append(QStringLiteral(">\n")).append(wrapper->popplerLog()).append(QStringLiteral("</body>\n"));
                else
                    bodyText.append(QStringLiteral("/>\n"));
            }

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
            logText.append(QStringLiteral("<meta>\n")).append(metaText).append(QStringLiteral("</meta>\n"));
        if (!headerText.isEmpty())
            logText.append(QStringLiteral("<header>\n")).append(headerText).append(QStringLiteral("</header>\n"));
        if (!bodyText.isEmpty())
            logText.append(bodyText);
        logText += QStringLiteral("</fileanalysis>\n");

        qint64 endTime = QDateTime::currentMSecsSinceEpoch();
        logText.prepend(QString("<fileanalysis filename=\"%1\" status=\"ok\" time=\"%2\">\n").arg(DocScan::xmlify(filename)).arg(endTime - startTime));

        emit analysisReport(logText);

        delete wrapper;
    } else
        emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" />\n").arg(filename));

    m_isAlive = false;
}
