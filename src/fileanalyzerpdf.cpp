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
#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>

#include "popplerwrapper.h"
#include "fileanalyzerpdf.h"
#include "watchdog.h"
#include "guessing.h"
#include "general.h"

static const int oneMinuteInMillisec = 60000;
static const int twoMinutesInMillisec = oneMinuteInMillisec * 2;
static const int fourMinutesInMillisec = oneMinuteInMillisec * 4;
static const int sixMinutesInMillisec = oneMinuteInMillisec * 6;

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

void FileAnalyzerPDF::setupPdfBoXValidator(const QString &pdfboxValidatorJavaClass) {
    m_pdfboxValidatorJavaClass = pdfboxValidatorJavaClass;
}

void FileAnalyzerPDF::setupCallasPdfAPilotCLI(const QString &callasPdfAPilotCLI) {
    m_callasPdfAPilotCLI = callasPdfAPilotCLI;
}

void FileAnalyzerPDF::analyzeFile(const QString &filename)
{
    if (filename.endsWith(QStringLiteral(".xz")) || filename.endsWith(QStringLiteral(".gz")) || filename.endsWith(QStringLiteral(".bz2")) || filename.endsWith(QStringLiteral(".lzma"))) {
        /// File is xz-compressed
        qWarning() << "Compressed files like " << filename << " should not directly send through this analyzer, but rather be uncompressed by FileAnalyzerMultiplexer first";
        m_isAlive = false;
        return;
    }

    m_isAlive = true;
    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    static const QStringList defaultArgumentsForNice = QStringList() << QStringLiteral("-n") << QStringLiteral("17") << QStringLiteral("ionice") << QStringLiteral("-c") << QStringLiteral("3");

    bool veraPDFStartedRun1 = false, veraPDFStartedRun2 = false;
    bool veraPDFIsPDF = false;
    bool veraPDFIsPDFA1B = false, veraPDFIsPDFA1A = false;
    QString veraPDFStandardOutput = QString::null;
    QString veraPDFErrorOutput = QString::null;
    long veraPDFfilesize = 0;
    int veraPDFExitCode = INT_MIN;
    QProcess veraPDF(this);
    if (!m_veraPDFcliTool.isEmpty()) {
        const QStringList arguments = QStringList(defaultArgumentsForNice) << m_veraPDFcliTool << QStringLiteral("-x") << QStringLiteral("-f") << QStringLiteral("1b") << QStringLiteral("--maxfailures") << QStringLiteral("1") << QStringLiteral("--format") << QStringLiteral("xml") << filename;
        veraPDF.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        veraPDFStartedRun1 = veraPDF.waitForStarted(twoMinutesInMillisec);
        if (!veraPDFStartedRun1)
            qWarning() << "Failed to start veraPDF with arguments " << arguments.join("_");
    }

    bool callasPdfAPilotStartedRun1 = false, callasPdfAPilotStartedRun2 = false;
    QString callasPdfAPilotStandardOutput = QString::null;
    QString callasPdfAPilotErrorOutput = QString::null;
    int callasPdfAPilotExitCode = INT_MIN;
    int callasPdfAPilotCountErrors = -1;
    int callasPdfAPilotCountWarnings = -1;
    char callasPdfAPilotPDFA1letter = '\0';
    QProcess callasPdfAPilot(this);
    if (!m_callasPdfAPilotCLI.isEmpty()) {
        const QStringList arguments = QStringList() << defaultArgumentsForNice << m_callasPdfAPilotCLI << QStringLiteral("--quickpdfinfo") << filename;
        callasPdfAPilot.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        callasPdfAPilotStartedRun1 = callasPdfAPilot.waitForStarted(oneMinuteInMillisec);
        if (!callasPdfAPilotStartedRun1)
            qWarning() << "Failed to start callas PDF/A Pilot with arguments " << arguments.join("_");
    }

    bool jhoveStarted = false;
    bool jhoveIsPDF = false;
    bool jhovePDFWellformed = false, jhovePDFValid = false;
    QString jhovePDFversion = QString::null;
    QString jhovePDFprofile = QString::null;
    QString jhoveStandardOutput = QString::null;
    QString jhoveErrorOutput = QString::null;
    int jhoveExitCode = INT_MIN;
    QProcess jhove(this);
    if (!m_jhoveShellscript.isEmpty() && !m_jhoveConfigFile.isEmpty()) {
        const QStringList arguments = QStringList(defaultArgumentsForNice) << QStringLiteral("/bin/bash") << m_jhoveShellscript << QStringLiteral("-c") << m_jhoveConfigFile << QStringLiteral("-m") << QStringLiteral("PDF-hul") << filename;
        jhove.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        jhoveStarted = jhove.waitForStarted(oneMinuteInMillisec);
        if (!jhoveStarted)
            qWarning() << "Failed to start jhove with arguments " << arguments.join("_");
    }

    bool pdfboxValidatorStarted = false;
    bool pdfboxValidatorValidPdf = false;
    QString pdfboxValidatorStandardOutput = QString::null;
    QString pdfboxValidatorErrorOutput = QString::null;
    int pdfboxValidatorExitCode = INT_MIN;
    QProcess pdfboxValidator(this);
    if (!m_pdfboxValidatorJavaClass.isEmpty()) {
        const QFileInfo fi(m_pdfboxValidatorJavaClass);
        const QDir dir = fi.dir();
        const QStringList jarFiles = dir.entryList(QStringList() << QStringLiteral("*.jar"), QDir::Files, QDir::Name);
        pdfboxValidator.setWorkingDirectory(dir.path());
        const QStringList arguments = QStringList(defaultArgumentsForNice) << QStringLiteral("java") << QStringLiteral("-cp") << QStringLiteral(".:") + jarFiles.join(':') << fi.fileName().remove(QStringLiteral(".class")) << filename;
        pdfboxValidator.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        pdfboxValidatorStarted = pdfboxValidator.waitForStarted(oneMinuteInMillisec);
        if (!pdfboxValidatorStarted)
            qWarning() << "Failed to start pdfbox Validator with arguments " << arguments.join("_");
    }

    if (veraPDFStartedRun1) {
        if (!veraPDF.waitForFinished(sixMinutesInMillisec))
            qWarning() << "Waiting for veraPDF failed or exceeded time limit";
        veraPDFExitCode = veraPDF.exitCode();
        veraPDFStandardOutput = QString::fromUtf8(veraPDF.readAllStandardOutput().constData());
        veraPDFErrorOutput = QString::fromUtf8(veraPDF.readAllStandardError().constData());
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

            if (veraPDFIsPDFA1B) {
                /// So, it is PDF-A/1b, then test for PDF-A/1a
                const QStringList arguments = QStringList(defaultArgumentsForNice) << m_veraPDFcliTool << QStringLiteral("-x") << QStringLiteral("-f") << QStringLiteral("1a") << QStringLiteral("--maxfailures") << QStringLiteral("1") << QStringLiteral("--format") << QStringLiteral("xml") << filename;
                veraPDF.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
                veraPDFStartedRun2 = veraPDF.waitForStarted(twoMinutesInMillisec);
                if (!veraPDFStartedRun2)
                    qWarning() << "Failed to start veraPDF with arguments " << arguments.join("_");
            } else
                qDebug() << "Skipping second run of veraPDF as file " << filename << "is not PDF/A-1b";
        } else
            qWarning() << "Execution of veraPDF failed for file " << filename << ": " << veraPDFErrorOutput;
    }

    if (callasPdfAPilotStartedRun1) {
        if (!callasPdfAPilot.waitForFinished(twoMinutesInMillisec))
            qWarning() << "Waiting for callas PDF/A Pilot failed or exceeded time limit";
        callasPdfAPilotExitCode = callasPdfAPilot.exitCode();
        callasPdfAPilotStandardOutput = QString::fromUtf8(callasPdfAPilot.readAllStandardOutput().constData());
        callasPdfAPilotErrorOutput = QString::fromUtf8(callasPdfAPilot.readAllStandardError().constData());

        if (callasPdfAPilotExitCode == 0 && !callasPdfAPilotStandardOutput.isEmpty()) {
            static const QRegularExpression rePDFA(QStringLiteral("\\bInfo\\s+PDFA\\s+PDF/A-1([ab])"));
            const QRegularExpressionMatch match = rePDFA.match(callasPdfAPilotStandardOutput.right(512));
            callasPdfAPilotPDFA1letter = match.hasMatch() ? match.captured(1).at(0).toLatin1() : '\0';
            if (callasPdfAPilotPDFA1letter == 'a' || callasPdfAPilotPDFA1letter == 'b') {
                /// Document claims to be PDF/A-1a or PDF/A-1b, so test for errors
                const QStringList arguments = QStringList(defaultArgumentsForNice) << m_callasPdfAPilotCLI << QStringLiteral("-a") << filename;
                callasPdfAPilot.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
                callasPdfAPilotStartedRun2 = callasPdfAPilot.waitForStarted(oneMinuteInMillisec);
                if (!callasPdfAPilotStartedRun2)
                    qWarning() << "Failed to start callas PDF/A Pilot with arguments " << arguments.join("_");
            } else
                qDebug() << "Skipping second run of callas PDF/A Pilot as file " << filename << "is not PDF/A-1";
        } else
            qWarning() << "Execution of callas PDF/A Pilot failed for file " << filename << ": " << callasPdfAPilotErrorOutput;
    }

    if (jhoveStarted) {
        if (!jhove.waitForFinished(fourMinutesInMillisec))
            qWarning() << "Waiting for jHove failed or exceeded time limit";
        jhoveExitCode = jhove.exitCode();
        jhoveStandardOutput = QString::fromUtf8(jhove.readAllStandardOutput().constData()).replace(QLatin1Char('\n'), QStringLiteral("###"));
        jhoveErrorOutput = QString::fromUtf8(jhove.readAllStandardError().constData()).replace(QLatin1Char('\n'), QStringLiteral("###"));
        if (jhoveExitCode == 0 && !jhoveStandardOutput.isEmpty()) {
            jhoveIsPDF = jhoveStandardOutput.contains(QStringLiteral("Format: PDF"));
            static const QRegExp pdfStatusRegExp(QStringLiteral("\\b*Status: ([^#]+)"));
            if (pdfStatusRegExp.indexIn(jhoveStandardOutput) >= 0) {
                jhovePDFWellformed = pdfStatusRegExp.cap(1).startsWith(QStringLiteral("Well-Formed"), Qt::CaseInsensitive);
                jhovePDFValid = pdfStatusRegExp.cap(1).endsWith(QStringLiteral("and valid"));
            }
            static const QRegExp pdfVersionRegExp(QStringLiteral("\\bVersion: ([^#]+)#"));
            jhovePDFversion = pdfVersionRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfVersionRegExp.cap(1) : QString::null;
            static const QRegExp pdfProfileRegExp(QStringLiteral("\\bProfile: ([^#]+)(#|$)"));
            jhovePDFprofile = pdfProfileRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfProfileRegExp.cap(1) : QString::null;
        } else
            qWarning() << "Execution of jHove failed for file " << filename << ": " << jhoveErrorOutput;
    }

    if (pdfboxValidatorStarted) {
        if (!pdfboxValidator.waitForFinished(twoMinutesInMillisec))
            qWarning() << "Waiting for pdfbox Validator failed or exceeded time limit";
        pdfboxValidatorExitCode = pdfboxValidator.exitCode();
        pdfboxValidatorStandardOutput = QString::fromUtf8(pdfboxValidator.readAllStandardOutput().constData());
        pdfboxValidatorErrorOutput = QString::fromUtf8(pdfboxValidator.readAllStandardError().constData());
        if (pdfboxValidatorExitCode == 0 && !pdfboxValidatorStandardOutput.isEmpty())
            pdfboxValidatorValidPdf = pdfboxValidatorStandardOutput.contains(QStringLiteral("is a valid PDF/A-1b file"));
        else
            qWarning() << "Execution of pdfbox Validator failed for file " << filename << ": " << pdfboxValidatorErrorOutput;
    }

    if (veraPDFStartedRun2) {
        if (!veraPDF.waitForFinished(sixMinutesInMillisec))
            qWarning() << "Waiting for veraPDF failed or exceeded time limit";
        veraPDFExitCode = veraPDF.exitCode();
        /// Some string magic to skip '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' from second output
        const QString newStdOut = QString::fromUtf8(veraPDF.readAllStandardOutput().constData());
        const int p = newStdOut.indexOf(QStringLiteral("?>"));
        veraPDFStandardOutput = veraPDFStandardOutput + QStringLiteral("\n") + (p > 1 ? newStdOut.mid(p + 2) : newStdOut);
        veraPDFErrorOutput = veraPDFErrorOutput + QStringLiteral("\n") + QString::fromUtf8(veraPDF.readAllStandardError().constData());
        if (veraPDFExitCode == 0) {
            const QString startOfOutput = veraPDFStandardOutput.left(2048);
            veraPDFIsPDFA1A = startOfOutput.contains(QStringLiteral(" flavour=\"PDFA_1_A\"")) && startOfOutput.contains(QStringLiteral(" isCompliant=\"true\""));
        } else
            qWarning() << "Execution of veraPDF failed for file " << filename << ": " << veraPDFErrorOutput;
    }

    if (callasPdfAPilotStartedRun2) {
        if (!callasPdfAPilot.waitForFinished(fourMinutesInMillisec))
            qWarning() << "Waiting for callas PDF/A Pilot failed or exceeded time limit";
        callasPdfAPilotExitCode = callasPdfAPilot.exitCode();
        callasPdfAPilotStandardOutput = callasPdfAPilotStandardOutput + QStringLiteral("\n") + QString::fromUtf8(callasPdfAPilot.readAllStandardOutput().constData());
        callasPdfAPilotErrorOutput = callasPdfAPilotErrorOutput + QStringLiteral("\n") + QString::fromUtf8(callasPdfAPilot.readAllStandardError().constData());
        if (callasPdfAPilotExitCode == 0) {
            static const QRegularExpression reSummary(QStringLiteral("\\bSummary\\t(Errors|Warnings)\\t(0|[1-9][0-9]*)\\b"));
            QRegularExpressionMatchIterator reIter = reSummary.globalMatch(callasPdfAPilotStandardOutput.right(512));
            while (reIter.hasNext()) {
                const QRegularExpressionMatch match = reIter.next();
                if (match.captured(1) == QStringLiteral("Errors")) {
                    bool ok = false;
                    callasPdfAPilotCountErrors = match.captured(2).toInt(&ok);
                    if (!ok) callasPdfAPilotCountErrors = -1;
                } else if (match.captured(1) == QStringLiteral("Warnings")) {
                    bool ok = false;
                    callasPdfAPilotCountWarnings = match.captured(2).toInt(&ok);
                    if (!ok) callasPdfAPilotCountWarnings = -1;
                }
            }
        } else
            qWarning() << "Execution of callas PDF/A Pilot failed for file " << filename << ": " << callasPdfAPilotErrorOutput;
    }

    const qint64 externalProgramsEndTime = QDateTime::currentMSecsSinceEpoch();

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

        if (jhoveExitCode > INT_MIN) {
            /// insert data from jHove
            metaText.append(QString(QStringLiteral("<jhove exitcode=\"%3\" pdf=\"%4\" wellformed=\"%1\" valid=\"%2\"")).arg(jhovePDFWellformed ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhovePDFValid ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhoveExitCode).arg(jhoveIsPDF ? QStringLiteral("yes") : QStringLiteral("no")));
            if (jhovePDFversion.isEmpty() && jhovePDFprofile.isEmpty() && jhoveStandardOutput.isEmpty() && jhoveErrorOutput.isEmpty())
                metaText.append(QStringLiteral(" />\n"));
            else {
                metaText.append(QStringLiteral(">\n"));
                if (!jhovePDFversion.isEmpty())
                    metaText.append(QString(QStringLiteral("<version>%1</version>\n")).arg(DocScan::xmlify(jhovePDFversion)));
                if (!jhovePDFprofile.isEmpty()) {
                    const bool isPDFA1a = jhovePDFprofile.contains(QStringLiteral("ISO PDF/A-1, Level A"));
                    const bool isPDFA1b = isPDFA1a || jhovePDFprofile.contains(QStringLiteral("ISO PDF/A-1, Level B"));
                    metaText.append(QString(QStringLiteral("<profile linear=\"%2\" tagged=\"%3\" pdfa1a=\"%4\" pdfa1b=\"%5\" pdfx3=\"%6\">%1</profile>\n")).arg(DocScan::xmlify(jhovePDFprofile)).arg(jhovePDFprofile.contains(QStringLiteral("Linearized PDF")) ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhovePDFprofile.contains(QStringLiteral("Tagged PDF")) ? QStringLiteral("yes") : QStringLiteral("no")).arg(isPDFA1a ? QStringLiteral("yes") : QStringLiteral("no")).arg(isPDFA1b ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhovePDFprofile.contains(QStringLiteral("ISO PDF/X-3")) ? QStringLiteral("yes") : QStringLiteral("no")));
                }
                if (m_jhoveVerbose && !jhoveStandardOutput.isEmpty())
                    metaText.append(QString(QStringLiteral("<output>%1</output>\n")).arg(DocScan::xmlify(jhoveStandardOutput.replace(QStringLiteral("###"), QStringLiteral("\n")))));
                if (!jhoveErrorOutput.isEmpty())
                    metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(jhoveErrorOutput.replace(QStringLiteral("###"), QStringLiteral("\n")))));
                metaText.append(QStringLiteral("</jhove>\n"));
            }
        } else
            metaText.append(QStringLiteral("<jhove><error>jHove failed to start or was never started</error></jhove>\n"));

        if (veraPDFExitCode > INT_MIN) {
            /// insert XML data from veraPDF
            const int p = veraPDFStandardOutput.indexOf(QStringLiteral("?>"));
            if (p > 0) {
                metaText.append(QString(QStringLiteral("<verapdf exitcode=\"%1\" pdf=\"%2\" pdfa1b=\"%3\" pdfa1a=\"%4\" filesize=\"%5\">\n")).arg(veraPDFExitCode).arg(veraPDFIsPDF ? QStringLiteral("yes") : QStringLiteral("no")).arg(veraPDFIsPDFA1B ? QStringLiteral("yes") : QStringLiteral("no")).arg(veraPDFIsPDFA1A ? QStringLiteral("yes") : QStringLiteral("no")).arg(veraPDFfilesize));
                if (!veraPDFStandardOutput.isEmpty())
                    metaText.append(veraPDFStandardOutput.mid(p + 3));
                else if (!veraPDFErrorOutput.isEmpty())
                    metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(veraPDFErrorOutput)));
                metaText.append(QStringLiteral("</verapdf>\n"));
            }
        } else
            metaText.append(QStringLiteral("<verapdf><error>veraPDF failed to start or was never started</error></verapdf>\n"));

        if (pdfboxValidatorExitCode > INT_MIN) {
            /// insert result from Apache's PDFBox
            metaText.append(QString(QStringLiteral("<pdfboxvalidator exitcode=\"%1\" pdfa1b=\"%2\">\n")).arg(pdfboxValidatorExitCode).arg(pdfboxValidatorValidPdf ? QStringLiteral("yes") : QStringLiteral("no")));
            if (!pdfboxValidatorStandardOutput.isEmpty())
                metaText.append(DocScan::xmlify(pdfboxValidatorStandardOutput));
            else if (!pdfboxValidatorErrorOutput.isEmpty())
                metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(pdfboxValidatorErrorOutput)));
            metaText.append(QStringLiteral("</pdfboxvalidator>\n"));
        } else
            metaText.append(QStringLiteral("<pdfboxvalidator><error>pdfbox Validator failed to start or was never started</error></pdfboxvalidator>\n"));

        if (callasPdfAPilotExitCode > INT_MIN) {
            const bool isPDFA1a = callasPdfAPilotPDFA1letter == 'a' && callasPdfAPilotCountErrors == 0 && callasPdfAPilotCountWarnings == 0;
            const bool isPDFA1b = isPDFA1a || (callasPdfAPilotPDFA1letter == 'b' && callasPdfAPilotCountErrors == 0 && callasPdfAPilotCountWarnings == 0);
            metaText.append(QString(QStringLiteral("<callaspdfapilot exitcode=\"%1\" pdfa1b=\"%2\" pdfa1a=\"%3\">\n")).arg(callasPdfAPilotExitCode).arg(isPDFA1b ? QStringLiteral("yes") : QStringLiteral("no")).arg(isPDFA1a ? QStringLiteral("yes") : QStringLiteral("no")));
            if (!callasPdfAPilotStandardOutput.isEmpty())
                metaText.append(DocScan::xmlify(callasPdfAPilotStandardOutput));
            else if (!callasPdfAPilotErrorOutput.isEmpty())
                metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(callasPdfAPilotErrorOutput)));
            metaText.append(QStringLiteral("</callaspdfapilot>"));
        } else
            metaText.append(QStringLiteral("<callaspdfapilot><error>callas PDF/A Pilot failed to start or was never started</error></callaspdfapilot>\n"));

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

        const qint64 endTime = QDateTime::currentMSecsSinceEpoch();
        logText.prepend(QString("<fileanalysis filename=\"%1\" status=\"ok\" time=\"%2\" external_time=\"%3\">\n").arg(DocScan::xmlify(filename)).arg(endTime - startTime).arg(externalProgramsEndTime - startTime));

        emit analysisReport(logText);

        delete wrapper;
    } else
        emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" external_time=\"%2\" />\n").arg(filename).arg(externalProgramsEndTime - startTime));


    m_isAlive = false;
}
