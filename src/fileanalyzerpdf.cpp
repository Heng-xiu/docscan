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

#include "fileanalyzerpdf.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QXmlQuery>
#include <QRegularExpression>
#include <QStandardPaths>

#include "watchdog.h"
#include "guessing.h"
#include "general.h"

static const int oneMinuteInMillisec = 60000;
static const int twoMinutesInMillisec = oneMinuteInMillisec * 2;
static const int fourMinutesInMillisec = oneMinuteInMillisec * 4;
static const int sixMinutesInMillisec = oneMinuteInMillisec * 6;
static const int tenMinutesInMillisec = oneMinuteInMillisec * 10;
static const int twentyMinutesInMillisec = oneMinuteInMillisec * 20;
static const int thirtyMinutesInMillisec = oneMinuteInMillisec * 30;
static const int sixtyMinutesInMillisec = oneMinuteInMillisec * 60;

FileAnalyzerPDF::FileAnalyzerPDF(QObject *parent)
    : FileAnalyzerAbstract(parent), JHoveWrapper(), m_isAlive(false), m_validateOnlyPDFAfiles(false), m_downgradeToPDFA1b(false), m_enforcedValidationLevel(xmpNone), m_tempDirDowngradeToPDFA1b(QDir::tempPath() + QStringLiteral("/fileanalyzerPDF-downgradeToPDFA1b.d-XXXXXX"))
{
    FileAnalyzerAbstract::setObjectName(QStringLiteral("fileanalyzerpdf"));


    QTimer::singleShot(50, this, &FileAnalyzerPDF::delayedToolcheck);
}

void FileAnalyzerPDF::delayedToolcheck() {
    QProcess exiftoolprocess(this);
    const QStringList exiftoolarguments = QStringList() << QStringLiteral("-ver");
    QByteArray exiftoolStandardOutput;
    connect(&exiftoolprocess, &QProcess::readyReadStandardOutput, [&exiftoolprocess, &exiftoolStandardOutput]() {
        const QByteArray d(exiftoolprocess.readAllStandardOutput());
        exiftoolStandardOutput.append(d);
    });
    exiftoolprocess.start(QStringLiteral("exiftool"), exiftoolarguments, QIODevice::ReadOnly);
    if (!exiftoolprocess.waitForStarted(oneMinuteInMillisec) || !exiftoolprocess.waitForFinished(twoMinutesInMillisec)) {
        const QString report = QString(QStringLiteral("<toolcheck name=\"exiftool\" status=\"error\" exitcode=\"%1\" />\n")).arg(exiftoolprocess.exitCode());
        emit analysisReport(objectName(), report);
    } else {
        const QString stdout = QString::fromLocal8Bit(exiftoolStandardOutput).trimmed();
        const QString report = QString(QStringLiteral("<toolcheck name=\"exiftool\" status=\"ok\" exitcode=\"%1\" version=\"%2\" />\n")).arg(exiftoolprocess.exitCode()).arg(stdout);
        emit analysisReport(objectName(), report);
    }

    QProcess pdfinfoprocess(this);
    const QStringList pdfinfoarguments = QStringList() << QStringLiteral("-v");
    QByteArray pdfinfoStandardError;
    connect(&pdfinfoprocess, &QProcess::readyReadStandardError, [&pdfinfoprocess, &pdfinfoStandardError]() {
        const QByteArray d(pdfinfoprocess.readAllStandardError());
        pdfinfoStandardError.append(d);
    });
    pdfinfoprocess.start(QStringLiteral("pdfinfo"), pdfinfoarguments, QIODevice::ReadOnly);
    if (!pdfinfoprocess.waitForStarted(oneMinuteInMillisec) || !pdfinfoprocess.waitForFinished(twoMinutesInMillisec)) {
        const QString report = QString(QStringLiteral("<toolcheck name=\"pdfinfo\" status=\"error\" exitcode=\"%1\" />\n")).arg(pdfinfoprocess.exitCode());
        emit analysisReport(objectName(), report);
    } else {
        const QString stderr = QString::fromLocal8Bit(pdfinfoStandardError).trimmed();
        const int p1 = stderr.indexOf(QStringLiteral(" version "));
        const int p2 = p1 > 5 ? stderr.indexOf(QLatin1Char('\n'), p1 + 9) : -1;
        if (p1 > 5 && p2 > p1) {
            const QString versionNumber = stderr.mid(p1 + 9, p2 - p1 - 9);
            const QString report = QString(QStringLiteral("<toolcheck name=\"pdfinfo\" status=\"ok\" exitcode=\"%1\" version=\"%2\" />\n")).arg(pdfinfoprocess.exitCode()).arg(versionNumber);
            emit analysisReport(objectName(), report);
        } else {
            const QString report = QString(QStringLiteral("<toolcheck name=\"pdfinfo\" status=\"error\" exitcode=\"%1\"><error>Could not determine version number</error></toolcheck>\n")).arg(pdfinfoprocess.exitCode());
            emit analysisReport(objectName(), report);
        }
    }
}

bool FileAnalyzerPDF::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerPDF::setupJhove(const QString &shellscript) {
    const QString report = JHoveWrapper::setupJhove(this, shellscript);
    if (!report.isEmpty()) {
        /// This was the first time 'setupJhove' inherited from JHoveWrapper was called
        /// and it gave us a report back. Propagate this report to the logging system
        emit analysisReport(QStringLiteral("jhovewrapper"), report);
    }
}

void FileAnalyzerPDF::setupVeraPDF(const QString &cliTool)
{
    const QFileInfo fi(cliTool);
    if (fi.isFile() && fi.isExecutable()) {
        m_veraPDFcliTool = cliTool;

        QProcess veraPDF(this);
        const QStringList arguments = QStringList() << QStringLiteral("--version");
        veraPDF.start(m_veraPDFcliTool, arguments, QIODevice::ReadOnly);
        const bool veraPDFStarted = veraPDF.waitForStarted(oneMinuteInMillisec);
        if (!veraPDFStarted)
            qWarning() << "Failed to start veraPDF to retrieve version";
        else {
            const bool veraPDFExited = veraPDF.waitForFinished(oneMinuteInMillisec);
            if (!veraPDFExited)
                qWarning() << "Failed to finish veraPDF to retrieve version";
            else {
                const int veraPDFExitCode = veraPDF.exitCode();
                const QString veraPDFStandardOutput = QString::fromUtf8(veraPDF.readAllStandardOutput().constData()).trimmed();
                const QString veraPDFErrorOutput = QString::fromUtf8(veraPDF.readAllStandardError().constData()).trimmed();
                static const QRegularExpression regExpVersionNumber(QStringLiteral("veraPDF (([0-9]+[.])+[0-9]+)"));
                const QRegularExpressionMatch regExpVersionNumberMatch = regExpVersionNumber.match(veraPDFStandardOutput);
                const bool status = veraPDFExitCode == 0 && regExpVersionNumberMatch.hasMatch();
                const QString versionNumber = status ? regExpVersionNumberMatch.captured(1) : QString();

                QString report = QString(QStringLiteral("<toolcheck name=\"verapdf\" exitcode=\"%1\" status=\"%2\"%3>\n")).arg(veraPDFExitCode).arg(status ? QStringLiteral("ok") : QStringLiteral("error")).arg(status ? QString(QStringLiteral(" version=\"%1\"")).arg(versionNumber) : QString());
                if (!veraPDFStandardOutput.isEmpty())
                    report.append(QStringLiteral("<output>")).append(DocScan::xmlifyLines(veraPDFStandardOutput)).append(QStringLiteral("</output>\n"));
                if (!veraPDFErrorOutput.isEmpty())
                    report.append(QStringLiteral("<error>")).append(DocScan::xmlifyLines(veraPDFErrorOutput)).append(QStringLiteral("</error>\n"));
                report.append(QStringLiteral("</toolcheck>\n"));
                emit analysisReport(objectName(), report);
            }
        }
    } else
        qWarning() << "Program file for veraPDF does not exist or not executable:" << cliTool;
}

void FileAnalyzerPDF::setupPdfBoXValidator(const QString &pdfboxValidatorJavaClass) {
    const QFileInfo fi(pdfboxValidatorJavaClass);
    if (fi.isFile()) {
        m_pdfboxValidatorJavaClass = pdfboxValidatorJavaClass;
        const QDir dir = fi.dir();
        const QStringList jarList = dir.entryList(QStringList() << QStringLiteral("pdfbox-*.jar"), QDir::Files);
        static const QRegularExpression regExpVersionNumber(QStringLiteral("pdfbox-(([0-9]+[.])+[0-9]+)\\.jar"));
        const QRegularExpressionMatch regExpVersionNumberMatch = jarList.count() > 0 ? regExpVersionNumber.match(jarList.first()) : QRegularExpressionMatch();
        const bool status = jarList.count() == 1 && regExpVersionNumberMatch.hasMatch();
        const QString versionNumber = status ? regExpVersionNumberMatch.captured(1) : QString();
        const QString report = QString(QStringLiteral("<toolcheck name=\"pdfboxvalidator\" status=\"%1\"%2 />\n")).arg(status ? QStringLiteral("ok") : QStringLiteral("error")).arg(status ? QString(QStringLiteral(" version=\"%1\"")).arg(versionNumber) : QString());
        emit analysisReport(objectName(), report);
    } else
        qWarning() << "Class file for Apache PDFBox Validator does not exist:" << pdfboxValidatorJavaClass;
}

void FileAnalyzerPDF::setupCallasPdfAPilotCLI(const QString &callasPdfAPilotCLI) {
    const QFileInfo fi(callasPdfAPilotCLI);
    if (fi.isFile() && fi.isExecutable()) {
        m_callasPdfAPilotCLI = callasPdfAPilotCLI;

        // TODO version number logging
    } else
        qWarning() << "Program file for Callas PDF/A Pilot does not exist or not executable:" << callasPdfAPilotCLI;
}

void FileAnalyzerPDF::setupAdobePreflightReportDirectory(const QString &adobePreflightReportDirectory) {
    m_adobePreflightReportDirectory = adobePreflightReportDirectory;
    QFileInfo directory(m_adobePreflightReportDirectory);
    if (directory.exists() && directory.isReadable())
        emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"adobepreflightreportdirectory\" status=\"ok\"><directory>%1</directory></toolcheck>")).arg(DocScan::xmlify(directory.absoluteFilePath())));
    else
        emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"adobepreflightreportdirectory\" status=\"error\"><directory>%1</directory><error>Directory is inaccessible or does not exist</error></toolcheck>")).arg(DocScan::xmlify(directory.absoluteFilePath())));
}

void FileAnalyzerPDF::setupQoppaJPDFPreflightDirectory(const QString &qoppaJPDFPreflightDirectory) {
    m_qoppaJPDFPreflightDirectory = qoppaJPDFPreflightDirectory;
    QFileInfo directory(m_qoppaJPDFPreflightDirectory);
    if (directory.exists() && directory.isReadable()) {
        QProcess qoppaJPDFPreflightProcess(this);
        qoppaJPDFPreflightProcess.setWorkingDirectory(m_qoppaJPDFPreflightDirectory);
        QByteArray standardError;
        connect(&qoppaJPDFPreflightProcess, &QProcess::readyReadStandardError, [&qoppaJPDFPreflightProcess, &standardError]() {
            const QByteArray d(qoppaJPDFPreflightProcess.readAllStandardError());
            standardError.append(d);
        });
        qoppaJPDFPreflightProcess.start(m_qoppaJPDFPreflightDirectory + QStringLiteral("/ValidatePDFA1b.sh"), QStringList(), QIODevice::ReadOnly);
        if (qoppaJPDFPreflightProcess.waitForStarted(oneMinuteInMillisec) && qoppaJPDFPreflightProcess.waitForFinished(oneMinuteInMillisec)) {
            QTextStream ts(&standardError);
            const QString errorText = ts.readLine();
            const QString versionString = errorText.indexOf("Version: jPDFPreflight ") == 0 ? errorText.mid(23) : QString();
            if (versionString.isEmpty()) {
                qWarning() << "Failed to read version number from this string:" << errorText;
                emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"qoppajpdfpreflightdirectory\" status=\"error\"><directory>%1</directory><error>Failed to read version number</error></toolcheck>")).arg(DocScan::xmlify(directory.absoluteFilePath())));
            } else
                emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"qoppajpdfpreflightdirectory\" status=\"ok\" version=\"%2\"><directory>%1</directory></toolcheck>")).arg(DocScan::xmlify(directory.absoluteFilePath()), DocScan::xmlify(versionString)));
        } else {
            qWarning() << "Failed to start Qoppa jPDFPreflight: " << qoppaJPDFPreflightProcess.program() << qoppaJPDFPreflightProcess.arguments().join(' ') << " in directory " << qoppaJPDFPreflightProcess.workingDirectory();
            emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"qoppajpdfpreflightdirectory\" status=\"error\"><directory>%1</directory><error>Failed to start Qoppa jPDFPreflight</error></toolcheck>")).arg(DocScan::xmlify(directory.absoluteFilePath())));
        }
    } else
        emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"qoppajpdfpreflightdirectory\" status=\"error\"><directory>%1</directory><error>Directory is inaccessible or does not exist</error></toolcheck>")).arg(DocScan::xmlify(directory.absoluteFilePath())));
}

void FileAnalyzerPDF::setupThreeHeightsValidatorShellCLI(const QString &threeHeightsValidatorShellCLI, const QString &threeHeightsValidatorLicenseKey) {
    QFileInfo threeHeightsFileInfo(threeHeightsValidatorShellCLI);
    m_threeHeightsValidatorLicenseKey = threeHeightsValidatorLicenseKey;
    m_threeHeightsValidatorShellCLI = threeHeightsFileInfo.absoluteFilePath();
    if (threeHeightsFileInfo.exists() && threeHeightsFileInfo.isExecutable()) {
        QProcess threeHeightsProcess(this);
        QByteArray standardOutput;
        connect(&threeHeightsProcess, &QProcess::readyReadStandardOutput, [&threeHeightsProcess, &standardOutput]() {
            const QByteArray d(threeHeightsProcess.readAllStandardOutput());
            standardOutput.append(d);
        });
        static const QStringList arguments = QStringList() << QStringLiteral("-lk") << threeHeightsValidatorLicenseKey;
        threeHeightsProcess.start(m_threeHeightsValidatorShellCLI, arguments, QIODevice::ReadOnly);
        if (threeHeightsProcess.waitForStarted(oneMinuteInMillisec) && threeHeightsProcess.waitForFinished(oneMinuteInMillisec)) {
            QTextStream ts(&standardOutput);
            const QString firstLine = ts.readLine();
            if (firstLine.indexOf(QStringLiteral("3-Heights(TM) PDF Validator Shell. Version")) == 0) {
                const int spacePos = firstLine.indexOf(QLatin1Char(' '), 44);
                const QString versionString = firstLine.mid(43, spacePos - 43);
                emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"threeheightsvalidatorshellcli\" status=\"ok\" version=\"%2\"><path>%1</path></toolcheck>")).arg(DocScan::xmlify(m_threeHeightsValidatorShellCLI), DocScan::xmlify(versionString)));
            } else {
                qWarning() << "Could not find version string when executing 3-Heights PDF Validator Shell: " << threeHeightsProcess.program() << threeHeightsProcess.arguments().join(' ') << " in directory " << threeHeightsProcess.workingDirectory();
                emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"threeheightsvalidatorshellcli\" status=\"error\"><path>%1</path><error>Could not find version string when executing 3-Heights PDF Validator Shell</error></toolcheck>")).arg(DocScan::xmlify(m_threeHeightsValidatorShellCLI)));
            }
        } else {
            qWarning() << "Failed to start 3-Heights PDF Validator Shell: " << threeHeightsProcess.program() << threeHeightsProcess.arguments().join(' ') << " in directory " << threeHeightsProcess.workingDirectory();
            emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"threeheightsvalidatorshellcli\" status=\"error\"><path>%1</path><error>Failed to start 3-Heights PDF Validator Shell</error></toolcheck>")).arg(DocScan::xmlify(m_threeHeightsValidatorShellCLI)));
        }
    } else {
        qWarning() << "Binary for 3-Heights PDF Validator Shell does not exist or is not executable:" << m_threeHeightsValidatorShellCLI;
        emit analysisReport(objectName(), QString(QStringLiteral("<toolcheck name=\"threeheightsvalidatorshellcli\" status=\"error\"><path>%1</path><error>Binary for 3-Heights PDF Validator Shell does not exist or is not executable</error></toolcheck>")).arg(DocScan::xmlify(m_threeHeightsValidatorShellCLI)));
    }
}

void FileAnalyzerPDF::setAliasName(const QString &toAnalyzeFilename, const QString &aliasFilename) {
    m_toAnalyzeFilename = toAnalyzeFilename;
    m_aliasFilename = aliasFilename;
}

void FileAnalyzerPDF::setPDFAValidationOptions(const bool validateOnlyPDFAfiles, const bool downgradeToPDFA1b, const XMPPDFConformance enforcedValidationLevel) {
    m_validateOnlyPDFAfiles = validateOnlyPDFAfiles;
    m_downgradeToPDFA1b = downgradeToPDFA1b;
    m_enforcedValidationLevel = enforcedValidationLevel;
}

bool FileAnalyzerPDF::adobePreflightReportAnalysis(const QString &filename, QString &metaText) {
    if (m_adobePreflightReportDirectory.isEmpty()) return false; ///< no report directory set
    const QDir startDirectory(m_adobePreflightReportDirectory);
    if (!startDirectory.exists()) return false; ///< report directory does not exist

    /// Perform a iterative approach to recursively scan the given report directory
    /// for a XML file (plain or compressed) matching the PDF filename (.pdf -> _report.xml)
    QVector<QDir> directoryQueue = QVector<QDir>() << startDirectory;
    const QStringList pattern = QStringList() << QFileInfo(filename).fileName().replace(QStringLiteral(".pdf"), QStringLiteral("_report.xml")).remove(QStringLiteral(".xz")).append("*");
    QString reportXMLfile;
    while (!directoryQueue.isEmpty() && reportXMLfile.isEmpty()) {
        const QDir curDir = directoryQueue.front();
        directoryQueue.removeFirst();
        const QFileInfoList list = curDir.entryInfoList(pattern, QDir::AllDirs | QDir::Files | QDir::NoDotDot | QDir::NoDot, QDir::Name | QDir::DirsLast);
        if (list.isEmpty()) continue;

        for (const QFileInfo &fi : list) {
            if (fi.isDir()) {
                const QDir d = QDir(fi.absoluteFilePath());
                directoryQueue.append(d);
            } else if (fi.isFile() && fi.isReadable()) {
                reportXMLfile = fi.absoluteFilePath();
                break;
            }
        }
    }
    if (reportXMLfile.isEmpty()) return false; ///< no report file found matching the PDF file

    QString xmlCode;
    if (reportXMLfile.endsWith(QStringLiteral(".xml"))) {
        /// Read uncompressed XML code into string xmlCode
        QFile f(reportXMLfile);
        if (f.open(QFile::ReadOnly)) {
            xmlCode = QString::fromUtf8(f.readAll());
            f.close();
        }
    } else if (reportXMLfile.endsWith(QStringLiteral(".xml.xz"))) {
        QFile f(reportXMLfile);
        if (f.open(QFile::ReadOnly)) {
            /// Read compressed XML code
            const QByteArray compressedData = f.readAll();
            f.close();

            QProcess unxzProcess(this);
            QByteArray standardOutput;
            connect(&unxzProcess, &QProcess::readyReadStandardOutput, [&unxzProcess, &standardOutput]() {
                /// store uncompressed XML data in standardOutput
                const QByteArray d(unxzProcess.readAllStandardOutput());
                standardOutput.append(d);
            });
            /// Start unxz, feed compressed data, wait for data to be processed
            unxzProcess.start(QStringLiteral("unxz"), QProcess::ReadWrite);
            if (unxzProcess.waitForStarted(oneMinuteInMillisec)
                    && unxzProcess.write(compressedData) > 0
                    && unxzProcess.waitForBytesWritten(oneMinuteInMillisec)) {
                unxzProcess.closeWriteChannel();
                if (unxzProcess.waitForFinished(tenMinutesInMillisec)
                        && unxzProcess.exitCode() == 0
                        && unxzProcess.exitStatus() == QProcess::NormalExit)
                    /// Assuming everything went fine, take standard output as recorded in above lambda function
                    /// and put it into string xmlCode
                    xmlCode = QString::fromUtf8(standardOutput);
            }
        }
    }

    if (xmlCode.isEmpty()) return false; ///< empty XML code means error

    /// Remove <? ... ?> header
    const int pq = xmlCode.indexOf(QStringLiteral("?>"));
    if (pq > 0) xmlCode = xmlCode.mid(pq + 2).trimmed();

    if (!xmlCode.startsWith(QStringLiteral("<report>")) || !xmlCode.endsWith(QStringLiteral("</report>")))
        /// Basic check on expected content failed
        return false;

    QXmlQuery query;
    query.setFocus(xmlCode);
    /// Query to count warning or error messages (count must be 0 to be a valid PDF/A-1b file)
    query.setQuery(QStringLiteral("count(//report/results/hits[@severity=\"Warning\" or @severity=\"Error\"])"));
    if (!query.isValid())
        return false;

    QString result;
    query.evaluateTo(&result); ///< apply query on XML object
    bool ok = false;
    int countWarningsErrors = result.toInt(&ok);
    if (!ok) return false; ///< result from query was not a number

    metaText.append(QString(QStringLiteral("<adobepreflight status=\"ok\" pdfa1b=\"%1\" errorwarningscount=\"%2\" />\n")).arg(countWarningsErrors == 0 ? QStringLiteral("yes") : QStringLiteral("no")).arg(countWarningsErrors));

    return true; ///< no issues? exit with success
}

bool FileAnalyzerPDF::popplerAnalysis(const QString &filename, QString &logText, QString &metaText) {
    Poppler::Document *popplerDocument = Poppler::Document::load(filename);
    const bool popplerWrapperOk = popplerDocument != nullptr;
    if (popplerWrapperOk) {
        QString guess, headerText;
        const int numPages = popplerDocument->numPages();

        /// file format including mime type and file format version
        int majorVersion = 0, minorVersion = 0;
        popplerDocument->getPdfVersion(&majorVersion, &minorVersion);
        metaText.append(QString(QStringLiteral("<fileformat>\n<mimetype>application/pdf</mimetype>\n<version major=\"%1\" minor=\"%2\">%1.%2</version>\n<security locked=\"%3\" encrypted=\"%4\" />\n</fileformat>\n")).arg(QString::number(majorVersion), QString::number(minorVersion), popplerDocument->isLocked() ? QStringLiteral("yes") : QStringLiteral("no"), popplerDocument->isEncrypted() ? QStringLiteral("yes") : QStringLiteral("no")));

        if (enableEmbeddedFilesAnalysis) {
            metaText.append(QStringLiteral("<embeddedfiles>\n"));
            metaText.append(QStringLiteral("<parentfilename>") + DocScan::xmlify(filename) + QStringLiteral("</parentfilename>\n"));
            extractImages(metaText, filename);
            extractEmbeddedFiles(metaText, popplerDocument);
            metaText.append(QStringLiteral("</embeddedfiles>\n"));
        }

        /// guess and evaluate editor (a.k.a. creator)
        QString toolXMLtext;
        QString creator = popplerDocument->info(QStringLiteral("Creator"));
        guess.clear();
        if (!creator.isEmpty())
            guess = guessTool(creator, popplerDocument->info(QStringLiteral("Title")));
        if (!guess.isEmpty())
            toolXMLtext.append(QString(QStringLiteral("<tool type=\"editor\">\n%1</tool>\n")).arg(guess));
        /// guess and evaluate producer
        QString producer = popplerDocument->info(QStringLiteral("Producer"));
        guess.clear();
        if (!producer.isEmpty())
            guess = guessTool(producer, popplerDocument->info(QStringLiteral("Title")));
        if (!guess.isEmpty())
            toolXMLtext.append(QString(QStringLiteral("<tool type=\"producer\">\n%1</tool>\n")).arg(guess));
        if (!toolXMLtext.isEmpty())
            metaText.append(QStringLiteral("<tools>\n")).append(toolXMLtext).append(QStringLiteral("</tools>\n"));

        QHash<QString, struct ExtendedFontInfo> knownFonts;
        for (int pageNumber = 0; pageNumber < numPages;) {
            Poppler::FontIterator *fontIterator = popplerDocument->newFontIterator(pageNumber);
            ++pageNumber;
            if (fontIterator == nullptr) continue;

            if (fontIterator->hasNext()) {
                const QList<Poppler::FontInfo> fontList = fontIterator->next();
                for (const Poppler::FontInfo &fi : fontList) {
                    const QString fontName = fi.name();
                    if (knownFonts.contains(fontName)) {
                        struct ExtendedFontInfo &efi = knownFonts[fontName];
                        efi.recordOccurrence(pageNumber);
                    } else {
                        const struct ExtendedFontInfo efi(fi, pageNumber);
                        knownFonts[fontName] = efi;
                    }
                }
            }
            delete fontIterator; ///< clean memory
        }
        QString fontXMLtext;
        for (QHash<QString, struct ExtendedFontInfo>::ConstIterator it = knownFonts.constBegin(); it != knownFonts.constEnd(); ++it) {
            bool oninnerpage = false;
            for (int pageNumber = 4; !oninnerpage && pageNumber < numPages - 4; ++pageNumber)
                oninnerpage = it.value().pageNumbers.contains(pageNumber);
            fontXMLtext.append(QString(QStringLiteral("<font firstpage=\"%5\" lastpage=\"%6\" oninnerpage=\"%7\" embedded=\"%2\" subset=\"%3\"%4>\n%1</font>\n")).arg(Guessing::fontToXML(it.value().name, it.value().typeName), it.value().isEmbedded ? QStringLiteral("yes") : QStringLiteral("no"), it.value().isSubset ? QStringLiteral("yes") : QStringLiteral("no"), it.value().fileName.isEmpty() ? QString() : QString(QStringLiteral(" filename=\"%1\"")).arg(it.value().fileName)).arg(it.value().firstPageNumber).arg(it.value().lastPageNumber).arg(oninnerpage ? QStringLiteral("yes") : QStringLiteral("no")));
        }
        if (!fontXMLtext.isEmpty())
            /// Wrap multiple <font> tags into one <fonts> tag
            metaText.append(QStringLiteral("<fonts>\n")).append(fontXMLtext).append(QStringLiteral("</fonts>\n"));

        /// format creation date
        QDate date = popplerDocument->date(QStringLiteral("CreationDate")).toUTC().date();
        if (date.isValid())
            headerText.append(DocScan::formatDate(date, creationDate));
        /// format modification date
        date = popplerDocument->date(("ModDate")).toUTC().date();
        if (date.isValid())
            headerText.append(DocScan::formatDate(date, modificationDate));

        /// retrieve author
        const QString author = popplerDocument->info(QStringLiteral("Author")).simplified();
        if (!author.isEmpty())
            headerText.append(QString(QStringLiteral("<author>%1</author>\n")).arg(DocScan::xmlify(author)));

        /// retrieve title
        QString title = popplerDocument->info(QStringLiteral("Title")).simplified();
        /// clean-up title
        if (microsoftToolRegExp.indexIn(title) == 0)
            title = microsoftToolRegExp.cap(3);
        if (!title.isEmpty())
            headerText.append(QString(QStringLiteral("<title>%1</title>\n")).arg(DocScan::xmlify(title)));

        /// retrieve subject
        const QString subject = popplerDocument->info(QStringLiteral("Subject")).simplified();
        if (!subject.isEmpty())
            headerText.append(QString(QStringLiteral("<subject>%1</subject>\n")).arg(DocScan::xmlify(subject)));

        /// retrieve keywords
        const QString keywords = popplerDocument->info(QStringLiteral("Keywords")).simplified();
        if (!keywords.isEmpty())
            headerText.append(QString(QStringLiteral("<keyword>%1</keyword>\n")).arg(DocScan::xmlify(keywords)));

        QString bodyText = QString(QStringLiteral("<body numpages=\"%1\"")).arg(numPages);
        if (textExtraction > teNone) {
            QString text;
            for (int i = 0; i < numPages; ++i)
                text += popplerDocument->page(i)->text(QRectF());
            bodyText.append(QString(QStringLiteral(" length=\"%1\"")).arg(text.length()));
            if (textExtraction >= teFullText) {
                bodyText.append(QStringLiteral(">\n"));
                if (textExtraction >= teAspell) {
                    const QString language = guessLanguage(text);
                    if (!language.isEmpty())
                        bodyText.append(QString(QStringLiteral("<language tool=\"aspell\">%1</language>\n")).arg(language));
                }
                bodyText.append(QStringLiteral("<text>")).append(DocScan::xmlifyLines(text)).append(QStringLiteral("</text>\n"));
                bodyText.append(QStringLiteral("</body>\n"));
            } else
                bodyText.append(QStringLiteral(" />\n"));
        } else
            bodyText.append(QStringLiteral(" />\n"));
        logText.append(bodyText);

        /// look into first page for info
        if (numPages > 0) {
            Poppler::Page *page = popplerDocument->page(0);
            const QSize size = page->pageSize();
            const int mmw = size.width() * 0.3527778;
            const int mmh = size.height() * 0.3527778;
            if (mmw > 0 && mmh > 0) {
                if (page->orientation() == Poppler::Page::Seascape || page->orientation() == Poppler::Page::Landscape)
                    headerText += evaluatePaperSize(mmh, mmw);
                if (page->orientation() == Poppler::Page::Portrait || page->orientation() == Poppler::Page::UpsideDown)
                    headerText += evaluatePaperSize(mmw, mmh);
            }
        }

        if (!headerText.isEmpty())
            logText.append(QStringLiteral("<header>\n")).append(headerText).append(QStringLiteral("</header>\n"));

        delete popplerDocument;
        return true;
    } else
        return false;
}

FileAnalyzerPDF::PDFVersion FileAnalyzerPDF::pdfVersionAnalysis(const QString &filename) {
    QFile pdfFile(filename);
    if (pdfFile.open(QFile::ReadOnly)) {
        const QByteArray header(pdfFile.read(16));
        pdfFile.close();
        if (header.size() < 16)
            return pdfVersionError;
        if (header[0] != '%' || header[1] != 'P' || header[2] != 'D' || header[3] != 'F' || header[4] != '-')
            return pdfVersionError;
        if (header[5] == '1' && header[6] == '.') {
            switch (header[7]) {
            case '1': return pdfVersion1dot1;
            case '2': return pdfVersion1dot2;
            case '3': return pdfVersion1dot3;
            case '4': return pdfVersion1dot4;
            case '5': return pdfVersion1dot5;
            case '6': return pdfVersion1dot6;
            case '7': return pdfVersion1dot7;
            default: return pdfVersionError;
            }
        } else if (header[5] == '2' && header[6] == '.') {
            switch (header[7]) {
            case '0': return pdfVersion2dot0;
            case '1': return pdfVersion2dot1;
            case '2': return pdfVersion2dot2;
            default: return pdfVersionError;
            }
        } else
            return pdfVersionError;
    } else
        return pdfVersionError;
}

inline QString FileAnalyzerPDF::pdfVersionToString(const FileAnalyzerPDF::PDFVersion pdfVersion) const {
    switch (pdfVersion) {
    case pdfVersionError: return QStringLiteral("invalid/error");
    case pdfVersion1dot1: return QStringLiteral("PDF 1.1");
    case pdfVersion1dot2: return QStringLiteral("PDF 1.2");
    case pdfVersion1dot3: return QStringLiteral("PDF 1.3");
    case pdfVersion1dot4: return QStringLiteral("PDF 1.4");
    case pdfVersion1dot5: return QStringLiteral("PDF 1.5");
    case pdfVersion1dot6: return QStringLiteral("PDF 1.6");
    case pdfVersion1dot7: return QStringLiteral("PDF 1.7");
    case pdfVersion2dot0: return QStringLiteral("PDF 2.0");
    case pdfVersion2dot1: return QStringLiteral("PDF 2.1");
    case pdfVersion2dot2: return QStringLiteral("PDF 2.2");
    }
    return QStringLiteral("invalid");
}

FileAnalyzerPDF::XMPPDFConformance FileAnalyzerPDF::xmpAnalysis(const QString &filename, const PDFVersion pdfVersion, QString &metaText) {
    metaText.append(QStringLiteral("<xmp>"));

    QByteArray metadataToolOutput;
    const QFileInfo fi(filename);
    /// Preferred tool to retrieve XMP Metadata is 'pdfinfo' from poppler
    QProcess pdfinfoprocess(this);
    pdfinfoprocess.setWorkingDirectory(fi.absolutePath());
    const QStringList pdfinfoarguments = QStringList() << QStringLiteral("-meta")  << filename;
    connect(&pdfinfoprocess, &QProcess::readyReadStandardOutput, [&pdfinfoprocess, &metadataToolOutput]() {
        const QByteArray d(pdfinfoprocess.readAllStandardOutput());
        metadataToolOutput.append(d);
    });
    pdfinfoprocess.start(QStringLiteral("pdfinfo"), pdfinfoarguments, QIODevice::ReadOnly);
    if (!pdfinfoprocess.waitForStarted(oneMinuteInMillisec) || !pdfinfoprocess.waitForFinished(twoMinutesInMillisec)) {
        metaText.append(QString(QStringLiteral("<warning>Failed to run 'pdfinfo', using 'exiftool' as fallback</warning>\n")));
        /// In case 'pdfinfo' fails or is not available, use 'exiftool' instead
        metadataToolOutput.clear();
        QProcess exiftoolprocess(this);
        exiftoolprocess.setWorkingDirectory(fi.absolutePath());
        const QStringList exiftoolarguments = QStringList() << QStringLiteral("-xmp")  << QStringLiteral("-b") << filename;
        connect(&exiftoolprocess, &QProcess::readyReadStandardOutput, [&exiftoolprocess, &metadataToolOutput]() {
            const QByteArray d(exiftoolprocess.readAllStandardOutput());
            metadataToolOutput.append(d);
        });
        exiftoolprocess.start(QStringLiteral("exiftool"), exiftoolarguments, QIODevice::ReadOnly);
        if (!exiftoolprocess.waitForStarted(oneMinuteInMillisec) || !exiftoolprocess.waitForFinished(twoMinutesInMillisec)) {
            metaText.append(QString(QStringLiteral("<error>Failed to run 'exiftool'</error></xmp>\n")));
            return xmpError;
        }
    }

    const QString output = QString::fromLocal8Bit(metadataToolOutput).trimmed();
    if (output.isEmpty()) {
        metaText.append(QString(QStringLiteral("<warning>Empty output from 'pdfinfo' or 'exiftool'</warning></xmp>\n")));
        return xmpError;
    }

    static const QString pdfPartTag(QStringLiteral("<pdfaid:part>"));
    const int pPdfPartTag = output.indexOf(pdfPartTag);
    QChar part;
    if (pdfPartTag > 0 && output[pPdfPartTag + 13] != QLatin1Char('<') && output[pPdfPartTag + 14] == QLatin1Char('<'))
        part = output[pPdfPartTag + 13];

    static const QString pdfConformanceTag(QStringLiteral("<pdfaid:conformance>"));
    const int pPdfConformanceTag = output.indexOf(pdfConformanceTag);
    QChar conformance;
    if (pPdfConformanceTag > 0 && output[pPdfConformanceTag + 20] != QLatin1Char('<') && output[pPdfConformanceTag + 21] == QLatin1Char('<'))
        conformance = output[pPdfConformanceTag + 20].toLower();

    if (part.isNull() && conformance.isNull()) {
        static const QString pdfPartAttribute(QStringLiteral(" pdfaid:part=")), pdfConformanceAttribute(QStringLiteral(" pdfaid:conformance="));
        const int pPdfPartAttribute = output.indexOf(pdfPartAttribute), pPdfConformanceAttribute = output.indexOf(pdfConformanceAttribute);
        if (pPdfPartAttribute > 0 && output[pPdfPartAttribute + 14] != QLatin1Char('"') && output[pPdfPartAttribute + 14] != QLatin1Char('\'') && (output[pPdfPartAttribute + 15] == QLatin1Char('"') || output[pPdfPartAttribute + 15] == QLatin1Char('\'')))
            part = output[pPdfPartAttribute + 14];
        if (pPdfConformanceAttribute > 0 && output[pPdfConformanceAttribute + 21] != QLatin1Char('"') && output[pPdfConformanceAttribute + 21] != QLatin1Char('\'') && (output[pPdfConformanceAttribute + 22] == QLatin1Char('"') || output[pPdfConformanceAttribute + 22] == QLatin1Char('\'')))
            conformance = output[pPdfConformanceAttribute + 21].toLower();
    }

    XMPPDFConformance xmpPDFConformance = xmpNone;

    if (part.isNull() || conformance.isNull())
        xmpPDFConformance = xmpNone;
    else if (part == QLatin1Char('1')) {
        if (conformance == QLatin1Char('a'))
            xmpPDFConformance = xmpPDFA1a;
        else if (conformance == QLatin1Char('b'))
            xmpPDFConformance = xmpPDFA1b;
    } else if (part == QLatin1Char('2')) {
        if (conformance == QLatin1Char('a'))
            xmpPDFConformance = xmpPDFA2a;
        else if (conformance == QLatin1Char('b'))
            xmpPDFConformance = xmpPDFA2b;
        else if (conformance == QLatin1Char('u'))
            xmpPDFConformance = xmpPDFA2u;
    } else if (part == QLatin1Char('3')) {
        if (conformance == QLatin1Char('a'))
            xmpPDFConformance = xmpPDFA3a;
        else if (conformance == QLatin1Char('b'))
            xmpPDFConformance = xmpPDFA3b;
        else if (conformance == QLatin1Char('u'))
            xmpPDFConformance = xmpPDFA3u;
    } else if (part == QLatin1Char('4'))
        xmpPDFConformance = xmpPDFA4;

    // TODO pdf:Producer xap:ModifyDate xap:CreateDate xap:CreatorTool xap:MetadataDate xapMM:DocumentID xapMM:InstanceID dc:title dc:creator

    metaText.append(QString(QStringLiteral("<pdfconformance pdfa1b=\"%1\" pdfa1a=\"%2\" pdfa2b=\"%3\" pdfa2a=\"%4\" pdfa2u=\"%5\" pdfa3b=\"%6\" pdfa3a=\"%7\" pdfa3u=\"%8\" pdfversionmatch=\"%10\">%9</pdfconformance></xmp>\n"))
                    .arg(xmpPDFConformance == xmpPDFA1b ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(xmpPDFConformance == xmpPDFA1a ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(xmpPDFConformance == xmpPDFA2b ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(xmpPDFConformance == xmpPDFA2a ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(xmpPDFConformance == xmpPDFA2u ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(xmpPDFConformance == xmpPDFA3b ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(xmpPDFConformance == xmpPDFA3a ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(xmpPDFConformance == xmpPDFA3u ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(xmpPDFConformanceToString(xmpPDFConformance))
                    .arg(pdfVersionMatchesXMPconformance(pdfVersion, xmpPDFConformance) ? QStringLiteral("yes") : QStringLiteral("no")));

    return xmpPDFConformance;
}

QString FileAnalyzerPDF::xmpPDFConformanceToString(const XMPPDFConformance xmpPDFConformance) const {
    switch (xmpPDFConformance) {
    case xmpError: return QStringLiteral("invalid");
    case xmpNone: return QString();
    case xmpPDFA1b: return QStringLiteral("PDF/A-1b");
    case xmpPDFA1a: return QStringLiteral("PDF/A-1a");
    case xmpPDFA2b: return QStringLiteral("PDF/A-2b");
    case xmpPDFA2a: return QStringLiteral("PDF/A-2a");
    case xmpPDFA2u: return QStringLiteral("PDF/A-2u");
    case xmpPDFA3b: return QStringLiteral("PDF/A-3b");
    case xmpPDFA3a: return QStringLiteral("PDF/A-3a");
    case xmpPDFA3u: return QStringLiteral("PDF/A-3u");
    case xmpPDFA4: return QStringLiteral("PDF/A-4");
    }
    return QStringLiteral("invalid");
}

bool FileAnalyzerPDF::pdfVersionMatchesXMPconformance(const FileAnalyzerPDF::PDFVersion pdfVersion, const FileAnalyzerPDF::XMPPDFConformance xmpPDFConformance) {
    return (pdfVersion == pdfVersion1dot4 && xmpPDFConformance >= xmpPDFA1b && xmpPDFConformance <= xmpPDFA1a)
           || (pdfVersion == pdfVersion1dot7 && xmpPDFConformance >= xmpPDFA2b && xmpPDFConformance <= xmpPDFA3u)
           || (pdfVersion == pdfVersion2dot0 && xmpPDFConformance == xmpPDFA4);
}

bool FileAnalyzerPDF::downgradingPDFA(const QString &filename) {
    static const QString docscanConformanceFakerPrefix(QStringLiteral("docscan-conformance-faker"));
    if (m_downgradeToPDFA1b && !filename.contains(docscanConformanceFakerPrefix)) {
        QFile pdfFile(filename);
        if (pdfFile.open(QFile::ReadOnly)) {
            QByteArray pdfData = pdfFile.readAll();
            pdfFile.close();
            bool doWrite = false;
            QString errorText;
            XMPPDFConformance originConformance = xmpNone;
            const int pPart1 = pdfData.indexOf("<pdfaid:part>1</pdfaid:part>");
            if (pPart1 > 0) {
                const int pConformanceA = pdfData.indexOf("<pdfaid:conformance>A</pdfaid:conformance>", qMax(0, pPart1 - 1024));
                if (pConformanceA > 0) {
                    pdfData[pConformanceA + 20] = 'B';
                    originConformance = xmpPDFA1a;
                    doWrite = true;
                }
            } else {
                const int pCombined = pdfData.indexOf(" pdfaid:part=\"1\" pdfaid:conformance=\"A\">");
                if (pCombined > 0) {
                    pdfData[pCombined + 39] = 'B';
                    originConformance = xmpPDFA1a;
                    doWrite = true;
                } else {
                    const int pPart2 = pdfData.indexOf("<pdfaid:part>2</pdfaid:part>");
                    if (pPart2 > 0) {
                        pdfData[pPart2 + 12] = '1';
                        doWrite = true;
                        const int pConformanceA = pdfData.indexOf("<pdfaid:conformance>A</pdfaid:conformance>", qMax(0, pPart2 - 1024));
                        const int pConformanceB = pdfData.indexOf("<pdfaid:conformance>B</pdfaid:conformance>", qMax(0, pPart2 - 1024));
                        const int pConformanceU = pdfData.indexOf("<pdfaid:conformance>U</pdfaid:conformance>", qMax(0, pPart2 - 1024));
                        if (pConformanceA > 0) {
                            pdfData[pConformanceA + 20] = 'B';
                            originConformance = xmpPDFA2a;
                        } else if (pConformanceB > 0)
                            originConformance = xmpPDFA2b;
                        else if (pConformanceU > 0) {
                            pdfData[pConformanceU + 20] = 'B';
                            originConformance = xmpPDFA2u;
                        }
                    } else {
                        const int pCombinedA = pdfData.indexOf(" pdfaid:part=\"2\" pdfaid:conformance=\"A\">");
                        const int pCombinedB = pdfData.indexOf(" pdfaid:part=\"2\" pdfaid:conformance=\"B\">");
                        const int pCombinedU = pdfData.indexOf(" pdfaid:part=\"2\" pdfaid:conformance=\"U\">");
                        if (pCombinedA > 0) {
                            pdfData[pCombinedA + 14] = '1';
                            pdfData[pCombinedA + 39] = 'B';
                            originConformance = xmpPDFA2a;
                            doWrite = true;
                        } else if (pCombinedB > 0) {
                            pdfData[pCombinedA + 14] = '1';
                            originConformance = xmpPDFA2b;
                            doWrite = true;
                        } else if (pCombinedU > 0) {
                            pdfData[pCombinedA + 14] = '1';
                            pdfData[pCombinedA + 39] = 'B';
                            originConformance = xmpPDFA2u;
                            doWrite = true;
                        } else {
                            errorText = QStringLiteral("Failed to identify XMP PDFaid data structure: either non-existent or compressed/encoded by Filters");
                            qWarning() << errorText;
                        }
                    }
                }
            }
            if (doWrite) {
                QString originalFilename = QFileInfo(m_aliasFilename.isEmpty() ? filename : m_aliasFilename).fileName().remove(QStringLiteral(".xz"));
                if (!originalFilename.isEmpty() && originalFilename[0] == QLatin1Char('.')) originalFilename = originalFilename.mid(1); ///< remove leading dot
#if QT_VERSION >= 0x050900
                const QString writeToFilename = m_tempDirDowngradeToPDFA1b.filePath(docscanConformanceFakerPrefix + QStringLiteral("-") + originalFilename);
#else // QT_VERSION >= 0x050900
                const QString writeToFilename = m_tempDirDowngradeToPDFA1b.path() + QLatin1Char('/') + docscanConformanceFakerPrefix + QStringLiteral("-") + originalFilename;
#endif // QT_VERSION >= 0x050900
                QFile pdfFile(writeToFilename);
                if (pdfFile.open(QFile::WriteOnly)) {
                    const QString originConformanceString = xmpPDFConformanceToString(originConformance);
                    const QString logText = QString(QStringLiteral("<downgradepdfa>\n<origin conformance=\"%3\">%1</origin>\n%4<destination conformance=\"PDF/A-1b\">%2</destination>\n</downgradepdfa>")).arg(DocScan::xmlify(filename), DocScan::xmlify(writeToFilename), DocScan::xmlify(originConformanceString), !m_aliasFilename.isEmpty() ? QStringLiteral("<alias>") + DocScan::xmlify(m_aliasFilename) + QStringLiteral("</alias>\n") : QString());
                    emit analysisReport(objectName(), logText);

                    pdfFile.write(pdfData);
                    pdfFile.close();

                    m_toAnalyzeFilename = writeToFilename;
                    m_aliasFilename.clear();
                    analyzeFile(writeToFilename);

                    pdfFile.remove();
                    return true;
                }
            } else if (!errorText.isEmpty()) {
                const QString logText = QString(QStringLiteral("<downgradepdfa><error>%1</error></downgradepdfa>")).arg(DocScan::xmlify(errorText));
                emit analysisReport(objectName(), logText);
            }
        }
    }
    return false;
}

void FileAnalyzerPDF::extractImages(QString &metaText, const QString &filename) {
    static const QString pdfimagesBinary = QStandardPaths::findExecutable(QStringLiteral("pdfimages"));
    if (pdfimagesBinary.isEmpty()) return; ///< no analysis without 'pdfimages' binary

    QProcess pdfimages(this);
    pdfimages.setWorkingDirectory(QStringLiteral("/tmp"));
    const QString prefix = QStringLiteral("docscan-pdf-extractimages-") + QString::number(qHash<QString>(filename, 0));
    const QStringList arguments = QStringList() << QStringLiteral("-j") << QStringLiteral("-jp2") << QStringLiteral("-ccitt") << QStringLiteral("-p") << QStringLiteral("-q") << filename << prefix;
    pdfimages.start(pdfimagesBinary, arguments, QIODevice::ReadOnly);
    if (!pdfimages.waitForStarted(twoMinutesInMillisec) || !pdfimages.waitForFinished(sixMinutesInMillisec))
        qWarning() << "Failed to start pdfimages for file " << filename << " and " << pdfimages.program() << pdfimages.arguments().join(' ') << " in directory " << pdfimages.workingDirectory();

    QDir dir(pdfimages.workingDirectory());
    const QStringList f = QStringList() << prefix + QStringLiteral("-*");
    const QStringList fileList = dir.entryList(f, QDir::Files, QDir::Name);
    for (const QString &imageFilename : fileList) {
        const QString absoluteImageFilename = pdfimages.workingDirectory() + QChar('/') + imageFilename;
        const QString fileExtension = imageFilename.mid(imageFilename.lastIndexOf(QChar('.')));
        if (blacklistedFileExtensions.contains(fileExtension)) {
            /// Skip this type of files
            QFile(absoluteImageFilename).remove();
            continue;
        }

        const QString mimetypeAsAttribute = QString(QStringLiteral(" mimetype=\"%1\"")).arg(DocScan::guessMimetype(imageFilename));

        emit foundEmbeddedFile(absoluteImageFilename);
        metaText.append(QStringLiteral("<embeddedfile") + mimetypeAsAttribute + QStringLiteral("><temporaryfilename>") + DocScan::xmlify(absoluteImageFilename) + QStringLiteral("</temporaryfilename></embeddedfile>"));
    }
}

void FileAnalyzerPDF::extractEmbeddedFiles(QString &metaText, Poppler::Document *popplerDocument) {
    const QList<Poppler::EmbeddedFile *> embeddedFiles = popplerDocument->embeddedFiles();
    if (!embeddedFiles.isEmpty()) {
        for (Poppler::EmbeddedFile *ef : embeddedFiles) {
            const QString fileExtension = ef->name().mid(ef->name().lastIndexOf(QChar('.')));
            if (blacklistedFileExtensions.contains(fileExtension)) {
                /// Skip this type of files
                continue;
            }

            const QString size = ef->size() >= 0 ? QString(QStringLiteral(" size=\"%1\"")).arg(ef->size()) : QString();
            const QString mimetype = ef->mimeType().isEmpty() ? DocScan::guessMimetype(ef->name()) : ef->mimeType();
            const QString mimetypeAsAttribute = QString(QStringLiteral(" mimetype=\"%1\"")).arg(mimetype);
            const QByteArray fileData = ef->data();
            const QString temporaryFilename = dataToTemporaryFile(fileData, mimetype);
            const QString embeddedFile = QStringLiteral("<embeddedfile") + size + mimetypeAsAttribute + QStringLiteral("><filename>") + DocScan::xmlify(ef->name()) + QStringLiteral("</filename>") + (ef->description().isEmpty() ? QString() : QStringLiteral("\n<description>") + DocScan::xmlify(ef->description()) + QStringLiteral("</description>")) + (temporaryFilename.isEmpty() ? QString() : QStringLiteral("<temporaryfilename>") + temporaryFilename /** no need for DocScan::xmlify */ + QStringLiteral("</temporaryfilename>")) + QStringLiteral("</embeddedfile>\n");
            metaText.append(embeddedFile);
            if (!temporaryFilename.isEmpty())
                emit foundEmbeddedFile(temporaryFilename);
        }
    }
}

void FileAnalyzerPDF::analyzeFile(const QString &filename)
{
    if (filename.endsWith(QStringLiteral(".xz")) || filename.endsWith(QStringLiteral(".gz")) || filename.endsWith(QStringLiteral(".bz2")) || filename.endsWith(QStringLiteral(".lzma"))) {
        /// File is compressed
        qWarning() << "Compressed files like " << filename << " should not directly send through this analyzer, but rather be uncompressed by FileAnalyzerMultiplexer first";
        m_isAlive = false;
        return;
    }

    m_isAlive = true;
    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    /// External programs should be both CPU and I/O 'nice'
    static const QStringList defaultArgumentsForNice = QStringList() << QStringLiteral("-n") << QStringLiteral("17") << QStringLiteral("ionice") << QStringLiteral("-c") << QStringLiteral("3");

    QString logText, metaText;
    metaText.reserve(16 * 1024 * 1024); ///< 16 MiB reserved
    const PDFVersion pdfVersion = pdfVersionAnalysis(filename);
    const XMPPDFConformance xmpPDFConformance = xmpAnalysis(filename, pdfVersion, metaText);

    /// While external programs run, analyze PDF file using the Poppler library
    const bool popplerWrapperOk = popplerAnalysis(filename, logText, metaText);

    /// If configured to do so, downgrade a PDF/A file that follows a PDF/A standard
    /// better than PDF/A-1b down to just PDF/A-1b by changing its metadata.
    /// This may be done in order to invoke PDF/A-1b-only validators on this file.
    /// The original file before the modification will not processed any further,
    /// unless the downgrading itself fails (e.g. because the metadata could not be
    /// changed).
    if (m_downgradeToPDFA1b && xmpPDFConformance != xmpPDFA1b && pdfVersionMatchesXMPconformance(pdfVersion, xmpPDFConformance)) {
        if (downgradingPDFA(filename)) {
            if (!metaText.isEmpty()) {
                metaText.squeeze();
                logText.append(QStringLiteral("<meta>\n")).append(metaText).append(QStringLiteral("</meta>\n"));
            }

            logText.prepend(QString(QStringLiteral("<fileanalysis filename=\"%1\" status=\"ok\">\n")).arg(DocScan::xmlify(filename)));
            logText.append(QStringLiteral("</fileanalysis>\n"));
            emit analysisReport(objectName(), logText);

            return;
        }
    }

    /// To save processing time, the user may choose to run validators on PDF files
    /// that look like valid PDF/A files on first sight. This 'first sight' is determined
    /// by the PDF version number in the magic string (first few bytes in file) and
    /// XMP metadata regarding PDF/A conformance part and level.
    /// For example, a file claiming to be PDF/A-1a must have PDF version 1.4.
    const bool doRunValidators = !m_validateOnlyPDFAfiles || pdfVersionMatchesXMPconformance(pdfVersion, xmpPDFConformance);

    QTemporaryDir veraPDFTemporaryDirectory(QDir::tempPath() + QStringLiteral("/.docscan-verapdf-"));
    bool veraPDFStartedRun = false;
    bool veraPDFIsPDFA1B = false, veraPDFIsPDFA1A = false;
    QByteArray veraPDFStandardOutputData, veraPDFStandardErrorData;
    QString veraPDFStandardOutput, veraPDFStandardError;
    QString veraPDFvalidationFlavor;
    long veraPDFfilesize = 0;
    int veraPDFExitCode = INT_MIN;
    QProcess veraPDF(this);
    veraPDF.setWorkingDirectory(veraPDFTemporaryDirectory.path());
    connect(&veraPDF, &QProcess::readyReadStandardOutput, [&veraPDF, &veraPDFStandardOutputData]() {
        const QByteArray d(veraPDF.readAllStandardOutput());
        veraPDFStandardOutputData.append(d);
    });
    connect(&veraPDF, &QProcess::readyReadStandardError, [&veraPDF, &veraPDFStandardErrorData]() {
        const QByteArray d(veraPDF.readAllStandardError());
        veraPDFStandardErrorData.append(d);
    });
    if (doRunValidators && !m_veraPDFcliTool.isEmpty()) {
        /// Chooses built-in Validation Profile flavour, e.g. '1b'
        veraPDFvalidationFlavor = (xmpPDFConformance == xmpPDFA1b || m_enforcedValidationLevel == xmpPDFA1b) ? QStringLiteral("1b") : (xmpPDFConformance == xmpPDFA1a ? QStringLiteral("1a") : (xmpPDFConformance == xmpPDFA2a ? QStringLiteral("2a") : (xmpPDFConformance == xmpPDFA2b ? QStringLiteral("2b") : (xmpPDFConformance == xmpPDFA2u ? QStringLiteral("2u") : QStringLiteral("1b") /** PDF/A-1b is fallback */))));
        const QStringList arguments = QStringList(defaultArgumentsForNice) << m_veraPDFcliTool << QStringLiteral("-x") << QStringLiteral("-f") << veraPDFvalidationFlavor << QStringLiteral("--maxfailures") << QStringLiteral("2048") << QStringLiteral("--verbose") << QStringLiteral("--format") << QStringLiteral("xml") << filename;
        veraPDF.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        veraPDFStartedRun = veraPDF.waitForStarted(twoMinutesInMillisec);
        if (!veraPDFStartedRun)
            qWarning() << "Failed to start veraPDF for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory();
    }

    bool threeHeightsPDFValidatorStartedRun = false;
    int  threeHeightsPDFValidatorExitCode = INT_MIN;
    QString threeHeightsPDFValidatorCLValue;
    QByteArray threeHeightsPDFValidatorStandardOutputData, threeHeightsPDFValidatorStandardErrorData;
    QString threeHeightsPDFValidatorStandardOutput, threeHeightsPDFValidatorStandardError;
    QProcess threeHeightsPDFValidatorProcess(this);
    connect(&threeHeightsPDFValidatorProcess, &QProcess::readyReadStandardOutput, [&threeHeightsPDFValidatorProcess, &threeHeightsPDFValidatorStandardOutputData]() {
        const QByteArray d(threeHeightsPDFValidatorProcess.readAllStandardOutput());
        threeHeightsPDFValidatorStandardOutputData.append(d);
    });
    connect(&threeHeightsPDFValidatorProcess, &QProcess::readyReadStandardError, [&threeHeightsPDFValidatorProcess, &threeHeightsPDFValidatorStandardErrorData]() {
        const QByteArray d(threeHeightsPDFValidatorProcess.readAllStandardError());
        threeHeightsPDFValidatorStandardErrorData.append(d);
    });
    if (doRunValidators && !m_threeHeightsValidatorShellCLI.isEmpty() && !m_threeHeightsValidatorLicenseKey.isEmpty()) {
        threeHeightsPDFValidatorCLValue = (xmpPDFConformance == xmpPDFA1b || m_enforcedValidationLevel == xmpPDFA1b) ? QStringLiteral("pdfa-1b") : (xmpPDFConformance == xmpPDFA1a ? QStringLiteral("pdfa-1a") : (xmpPDFConformance == xmpPDFA2a ? QStringLiteral("pdfa-2a") : (xmpPDFConformance == xmpPDFA2b ? QStringLiteral("pdfa-2b") : (xmpPDFConformance == xmpPDFA2u ? QStringLiteral("pdfa-2u") : QStringLiteral("ccl")))));
        const QStringList arguments = QStringList() << defaultArgumentsForNice << m_threeHeightsValidatorShellCLI << QStringLiteral("-lk") << m_threeHeightsValidatorLicenseKey << QStringLiteral("-cl") << threeHeightsPDFValidatorCLValue << QStringLiteral("-rd") << QStringLiteral("-rl") << QStringLiteral("3") << QStringLiteral("-v") << filename;
        threeHeightsPDFValidatorProcess.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        threeHeightsPDFValidatorStartedRun = threeHeightsPDFValidatorProcess.waitForStarted(oneMinuteInMillisec);
        if (!threeHeightsPDFValidatorStartedRun)
            qWarning() << "Failed to start 3-Heights PDF Validator Shell for file " << filename << " and " << threeHeightsPDFValidatorProcess.program() << threeHeightsPDFValidatorProcess.arguments().join(' ') << " in directory " << threeHeightsPDFValidatorProcess.workingDirectory();
    }

    bool callasPdfAPilotStartedRun1 = false, callasPdfAPilotStartedRun2 = false;
    int callasPdfAPilotExitCode = INT_MIN;
    int callasPdfAPilotCountErrors = -1;
    int callasPdfAPilotCountWarnings = -1;
    char callasPdfAPilotPDFA1letter = '\0';
    QString callasPdfAPilotStandardOutput, callasPdfAPilotStandardError;
    QByteArray callasPdfAPilotStandardOutputData, callasPdfAPilotStandardErrorData;
    QProcess callasPdfAPilot(this);
    connect(&callasPdfAPilot, &QProcess::readyReadStandardOutput, [&callasPdfAPilot, &callasPdfAPilotStandardOutputData]() {
        const QByteArray d(callasPdfAPilot.readAllStandardOutput());
        callasPdfAPilotStandardOutputData.append(d);
    });
    connect(&callasPdfAPilot, &QProcess::readyReadStandardError, [&callasPdfAPilot, &callasPdfAPilotStandardErrorData]() {
        const QByteArray d(callasPdfAPilot.readAllStandardError());
        callasPdfAPilotStandardErrorData.append(d);
    });
    if (doRunValidators && !m_callasPdfAPilotCLI.isEmpty()) {
        const QStringList arguments = QStringList() << defaultArgumentsForNice << m_callasPdfAPilotCLI << QStringLiteral("--quickpdfinfo") << filename;
        callasPdfAPilot.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        callasPdfAPilotStartedRun1 = callasPdfAPilot.waitForStarted(oneMinuteInMillisec);
        if (!callasPdfAPilotStartedRun1)
            qWarning() << "Failed to start callas PDF/A Pilot for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory();
    }

    bool qoppaJPDFPreflightStarted = false;
    int  qoppaJPDFPreflightExitCode = INT_MIN;
    QByteArray qoppaJPDFPreflightStandardOutputData, qoppaJPDFPreflightStandardErrorData;
    QString qoppaJPDFPreflightStandardOutput, qoppaJPDFPreflightStandardError;
    QString qoppaJPDFPreflightFlavor;
    QProcess qoppaJPDFPreflightProcess(this);
    qoppaJPDFPreflightProcess.setWorkingDirectory(m_qoppaJPDFPreflightDirectory);
    connect(&qoppaJPDFPreflightProcess, &QProcess::readyReadStandardOutput, [&qoppaJPDFPreflightProcess, &qoppaJPDFPreflightStandardOutputData]() {
        const QByteArray d(qoppaJPDFPreflightProcess.readAllStandardOutput());
        qoppaJPDFPreflightStandardOutputData.append(d);
    });
    connect(&qoppaJPDFPreflightProcess, &QProcess::readyReadStandardError, [&qoppaJPDFPreflightProcess, &qoppaJPDFPreflightStandardErrorData]() {
        const QByteArray d(qoppaJPDFPreflightProcess.readAllStandardError());
        qoppaJPDFPreflightStandardErrorData.append(d);
    });
    if (doRunValidators && !m_qoppaJPDFPreflightDirectory.isEmpty()) {
        qoppaJPDFPreflightFlavor = (xmpPDFConformance == xmpPDFA1a && m_enforcedValidationLevel == xmpNone) ?  QStringLiteral("PDFA1a") : QStringLiteral("PDFA1b");
        const QStringList arguments = QStringList() << defaultArgumentsForNice << (m_qoppaJPDFPreflightDirectory + QStringLiteral("/Validate") + qoppaJPDFPreflightFlavor + QStringLiteral(".sh")) << filename;
        qoppaJPDFPreflightProcess.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        qoppaJPDFPreflightStarted = qoppaJPDFPreflightProcess.waitForStarted(oneMinuteInMillisec);
        if (!qoppaJPDFPreflightStarted)
            qWarning() << "Failed to start Qoppa jPDFPreflight for file " << filename << " and " << qoppaJPDFPreflightProcess.program() << qoppaJPDFPreflightProcess.arguments().join(' ') << " in directory " << qoppaJPDFPreflightProcess.workingDirectory();
    }

    QProcess *jhoveProcess = doRunValidators ? launchJHove(this, JHovePDF, filename) : nullptr;
    QByteArray jhoveStandardOutputData, jhoveStandardErrorData;
    if (jhoveProcess != nullptr) {
        connect(jhoveProcess, &QProcess::readyReadStandardOutput, [jhoveProcess, &jhoveStandardOutputData]() {
            const QByteArray d(jhoveProcess->readAllStandardOutput());
            jhoveStandardOutputData.append(d);
        });
        connect(jhoveProcess, &QProcess::readyReadStandardError, [jhoveProcess, &jhoveStandardErrorData]() {
            const QByteArray d(jhoveProcess->readAllStandardError());
            jhoveStandardErrorData.append(d);
        });
    }
    const bool jhoveStarted = jhoveProcess != nullptr && jhoveProcess->waitForStarted(oneMinuteInMillisec);
    if (jhoveProcess != nullptr && !jhoveStarted)
        qWarning() << "Failed to start jhove for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory();

    bool pdfboxValidatorStarted = false;
    bool pdfboxValidatorValidPdf = false;
    int pdfboxValidatorExitCode = INT_MIN;
    QProcess pdfboxValidator(this);
    QByteArray pdfboxValidatorStandardOutputData, pdfboxValidatorStandardErrorData;
    QString pdfboxValidatorStandardOutput, pdfboxValidatorStandardError;
    connect(&pdfboxValidator, &QProcess::readyReadStandardOutput, [&pdfboxValidator, &pdfboxValidatorStandardOutputData]() {
        const QByteArray d(pdfboxValidator.readAllStandardOutput());
        pdfboxValidatorStandardOutputData.append(d);
    });
    connect(&pdfboxValidator, &QProcess::readyReadStandardError, [&pdfboxValidator, &pdfboxValidatorStandardErrorData]() {
        const QByteArray d(pdfboxValidator.readAllStandardError());
        pdfboxValidatorStandardErrorData.append(d);
    });
    if (doRunValidators && !m_pdfboxValidatorJavaClass.isEmpty()) {
        static const QFileInfo fi(m_pdfboxValidatorJavaClass);
        static const QDir dir = fi.dir();
        static const QStringList jarFiles = dir.entryList(QStringList() << QStringLiteral("*.jar"), QDir::Files, QDir::Name);
        pdfboxValidator.setWorkingDirectory(dir.path());
        const QStringList arguments = QStringList(defaultArgumentsForNice) << QStringLiteral("java") << QStringLiteral("-cp") << QStringLiteral(".:") + jarFiles.join(':') << fi.fileName().remove(QStringLiteral(".class")) << QStringLiteral("--xml") << filename;
        pdfboxValidator.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        pdfboxValidatorStarted = pdfboxValidator.waitForStarted(oneMinuteInMillisec);
        if (!pdfboxValidatorStarted)
            qWarning() << "Failed to start pdfbox Validator for file " << filename << " and " << pdfboxValidator.program() << pdfboxValidator.arguments().join(' ') << " in directory " << pdfboxValidator.workingDirectory() << ": " << QString::fromUtf8(pdfboxValidatorStandardErrorData.constData());
    }

    bool adobePreflightReportAnalysisOk = false;
    if (doRunValidators) {
        /// If for the current filename an alias filename was given,
        /// use the alias filename to locate the Adobe Preflight report.
        adobePreflightReportAnalysisOk = adobePreflightReportAnalysis(filename == m_toAnalyzeFilename && !m_aliasFilename.isEmpty() ? m_aliasFilename : m_toAnalyzeFilename, metaText);
        if (!adobePreflightReportAnalysisOk)
            metaText.append(QStringLiteral("<adobepreflight status=\"failed\"><error>Failed to find or evaluate Adobe Preflight XML report file</error></adobepreflight>\n"));
    } else
        metaText.append(QStringLiteral("<adobepreflight><info>Adobe Preflight not configured to run</info></adobepreflight>\n"));

    if (doRunValidators && veraPDFStartedRun) {
        static const int veraPDFtimeLimit = sixtyMinutesInMillisec;
        const bool veraPDFtimeExceeded = !veraPDF.waitForFinished(veraPDFtimeLimit);
        if (veraPDFtimeExceeded)
            qWarning() << "Waiting for veraPDF failed or exceeded time limit (" << (veraPDFtimeLimit / 1000) << "s) for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory();
        veraPDFExitCode = veraPDF.exitCode();
        veraPDFStandardOutput = DocScan::removeBinaryGarbage(QString::fromUtf8(veraPDFStandardOutputData.constData()).trimmed());
        /// Sometimes veraPDF does not return complete and valid XML code. veraPDF's bug or DocScan's bug?
        if ((!veraPDFStandardOutput.contains(QStringLiteral("<rawResults>")) || !veraPDFStandardOutput.contains(QStringLiteral("</rawResults>"))) && (!veraPDFStandardOutput.contains(QStringLiteral("<ns2:cliReport")) || !veraPDFStandardOutput.contains(QStringLiteral("</ns2:cliReport>"))))
            veraPDFStandardOutput = QString(QStringLiteral("<error exitcode=\"%1\" exceededtimelimit=\"%2\" timelimitsec=\"%3\">No matching opening and closing 'rawResults' or 'ns2:cliReport' tags found in output:\n")).arg(veraPDFExitCode).arg(veraPDFtimeExceeded ? QStringLiteral("yes") : QStringLiteral("no")).arg(veraPDFtimeLimit / 1000) + DocScan::xmlifyLines(veraPDFStandardOutput) + QStringLiteral("</error>");
        veraPDFStandardError = QString::fromUtf8(veraPDFStandardErrorData.constData()).trimmed();
        if (!veraPDFtimeExceeded && veraPDFExitCode == 0 && !veraPDFStandardOutput.isEmpty()) {
            const QString startOfOutput = veraPDFStandardOutput.left(8192);
            const int tagStart = startOfOutput.indexOf(QStringLiteral("<validationResult "));
            const int tagEnd = startOfOutput.indexOf(QStringLiteral(">"), tagStart + 10);
            const int flavourPos1 = startOfOutput.indexOf(QStringLiteral(" flavour=\""), tagStart + 10);
            const int isCompliantPos1 = startOfOutput.indexOf(QStringLiteral(" isCompliant=\""), tagStart + 10);
            if (flavourPos1 > 0 && isCompliantPos1 > 0 && flavourPos1 < tagEnd && isCompliantPos1 < tagEnd) {
                const int flavourPos2 = startOfOutput.indexOf(QStringLiteral("\""), flavourPos1 + 10);
                const int isCompliantPos2 = startOfOutput.indexOf(QStringLiteral("\""), isCompliantPos1 + 14);
                const bool complianceFlag = startOfOutput.mid(isCompliantPos1 + 14, isCompliantPos2 - isCompliantPos1 - 14) == QStringLiteral("true");
                if (complianceFlag) {
                    const QString flavor = startOfOutput.mid(flavourPos1 + 10, flavourPos2 - flavourPos1 - 10);
                    if (flavor == QStringLiteral("PDFA_1_B")) veraPDFIsPDFA1B = true;
                    else if (flavor == QStringLiteral("PDFA_1_A")) veraPDFIsPDFA1A = true;
                }
            }
            const int p4 = startOfOutput.indexOf(QStringLiteral("item size=\""));
            if (p4 > 1) {
                const int p5 = startOfOutput.indexOf(QStringLiteral("\""), p4 + 11);
                if (p5 > p4) {
                    bool ok = false;
                    veraPDFfilesize = startOfOutput.mid(p4 + 11, p5 - p4 - 11).toLong(&ok);
                    if (!ok) veraPDFfilesize = 0;
                }
            }
        } else
            qWarning() << "Execution of veraPDF failed for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory() << ": " << veraPDFStandardError;

        if (veraPDFtimeExceeded)
            veraPDF.kill();
    }

    if (doRunValidators && callasPdfAPilotStartedRun1) {
        static const int callasPdfAPilotTimeLimit = twentyMinutesInMillisec;
        const bool callasPdfAPilotTimeExceeded = !callasPdfAPilot.waitForFinished(callasPdfAPilotTimeLimit);
        if (callasPdfAPilotTimeExceeded)
            qWarning() << "Waiting for callas PDF/A Pilot failed or exceeded time limit (" << (callasPdfAPilotTimeLimit / 1000) << "s) for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory();
        callasPdfAPilotExitCode = callasPdfAPilot.exitCode();
        callasPdfAPilotStandardOutput = QString::fromUtf8(callasPdfAPilotStandardOutputData.constData()).trimmed();
        callasPdfAPilotStandardError = QString::fromUtf8(callasPdfAPilotStandardErrorData.constData()).trimmed();

        if (!callasPdfAPilotTimeExceeded && callasPdfAPilotExitCode == 0 && !callasPdfAPilotStandardOutput.isEmpty()) {
            static const QRegularExpression rePDFA(QStringLiteral("\\bInfo\\s+PDFA\\s+PDF/A-1([ab])"));
            const QRegularExpressionMatch match = rePDFA.match(callasPdfAPilotStandardOutput.right(512));
            callasPdfAPilotPDFA1letter = match.hasMatch() ? match.captured(1).at(0).toLatin1() : '\0';
            if (callasPdfAPilotPDFA1letter == 'a' || callasPdfAPilotPDFA1letter == 'b') {
                /// Document claims to be PDF/A-1a or PDF/A-1b, so test for errors
                callasPdfAPilotStandardOutputData.clear(); ///< reset before launching new PDF/A Pilot process
                callasPdfAPilotStandardErrorData.clear(); ///< reset before launching new PDF/A Pilot process
                const QStringList arguments = QStringList(defaultArgumentsForNice) << m_callasPdfAPilotCLI << QStringLiteral("-a") << filename;
                callasPdfAPilot.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
                callasPdfAPilotStartedRun2 = callasPdfAPilot.waitForStarted(oneMinuteInMillisec);
                if (!callasPdfAPilotStartedRun2)
                    qWarning() << "Failed to start callas PDF/A Pilot for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory();
            }
        } else {
            qWarning() << "Execution of callas PDF/A Pilot failed for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory() << ": " << callasPdfAPilotStandardError;
            callasPdfAPilotStandardOutput.prepend(QString(QStringLiteral("<error exitcode=\"%1\" exceededtimelimit=\"%2\" timelimitsec=\"%3\" />\n")).arg(callasPdfAPilotExitCode).arg(callasPdfAPilotTimeExceeded ? QStringLiteral("yes") : QStringLiteral("no")).arg(callasPdfAPilotTimeLimit / 1000));
        }

        if (callasPdfAPilotTimeExceeded)
            callasPdfAPilot.kill();
    }

    bool jhoveIsPDF = false;
    bool jhovePDFWellformed = false, jhovePDFValid = false;
    QString jhovePDFversion;
    QString jhovePDFprofile;
    int jhoveExitCode = INT_MIN;
    QString jhoveStandardOutput;
    QString jhoveStandardError;
    if (doRunValidators && jhoveStarted) {
        static const int jhoveTimeLimit = sixtyMinutesInMillisec;
        const bool jhoveTimeExceeded = !jhoveProcess->waitForFinished(jhoveTimeLimit);
        if (jhoveTimeExceeded)
            qWarning() << "Waiting for jHove failed or exceeded time limit (" << (jhoveTimeLimit / 1000) << "s) for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory();
        jhoveExitCode = jhoveProcess->exitCode();
        jhoveStandardOutput = QString::fromUtf8(jhoveStandardOutputData.constData());
        jhoveStandardError = QString::fromUtf8(jhoveStandardErrorData.constData());
        if (!jhoveTimeExceeded && jhoveExitCode == 0 && !jhoveStandardOutput.isEmpty()) {
            jhoveIsPDF = jhoveStandardOutput.contains(QStringLiteral("Format: PDF")) && !jhoveStandardOutput.contains(QStringLiteral("ErrorMessage:"));
            static const QRegExp pdfStatusRegExp(QStringLiteral("\\bStatus: ([-.0-9a-zA-Z ]+)"));
            if (pdfStatusRegExp.indexIn(jhoveStandardOutput) >= 0) {
                jhovePDFWellformed = pdfStatusRegExp.cap(1).startsWith(QStringLiteral("Well-Formed"), Qt::CaseInsensitive);
                jhovePDFValid = pdfStatusRegExp.cap(1).endsWith(QStringLiteral("and valid"));
            }
            static const QRegExp pdfVersionRegExp(QStringLiteral("\\bVersion: ([.0-9]+)"));
            jhovePDFversion = pdfVersionRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfVersionRegExp.cap(1) : QString();
            static const QRegExp pdfProfileRegExp(QStringLiteral("\\bProfile: ([-.,/0-9a-zA-Z ]+)"));
            jhovePDFprofile = pdfProfileRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfProfileRegExp.cap(1) : QString();
        } else {
            qWarning() << "Execution of jHove failed for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory() << ": " << jhoveStandardError;
            jhoveStandardOutput.prepend(QString(QStringLiteral("<error exitcode=\"%1\" exceededtimelimit=\"%2\" timelimitsec=\"%3\" />\n")).arg(jhoveExitCode).arg(jhoveTimeExceeded ? QStringLiteral("yes") : QStringLiteral("no")).arg(jhoveTimeLimit / 1000));
        }

        if (jhoveTimeExceeded)
            jhoveProcess->kill();
    }

    if (doRunValidators && pdfboxValidatorStarted) {
        static const int pdfboxValidatorTimeLimit = sixtyMinutesInMillisec;
        const bool pdfboxValidatorTimeExceeded = !pdfboxValidator.waitForFinished(pdfboxValidatorTimeLimit);
        if (pdfboxValidatorTimeExceeded)
            qWarning() << "Waiting for pdfbox Validator failed or exceeded time limit (" << (pdfboxValidatorTimeLimit / 1000) << "s) for file " << filename << " and " << pdfboxValidator.program() << pdfboxValidator.arguments().join(' ') << " in directory " << pdfboxValidator.workingDirectory();
        pdfboxValidatorExitCode =   pdfboxValidator.exitCode();
        pdfboxValidatorStandardOutput = QString::fromUtf8(DocScan::removeBinaryGarbage(pdfboxValidatorStandardOutputData).constData()).trimmed();
        pdfboxValidatorStandardError = QString::fromUtf8(DocScan::removeBinaryGarbage(pdfboxValidatorStandardErrorData).constData()).trimmed();
        if (!pdfboxValidatorTimeExceeded && pdfboxValidatorExitCode == 0 && !pdfboxValidatorStandardOutput.isEmpty())
            pdfboxValidatorValidPdf = pdfboxValidatorStandardOutput.contains(QStringLiteral("is a valid PDF/A-1b file"));
        else {
            qWarning() << "Execution of pdfbox Validator failed for file " << filename << " and " << pdfboxValidator.program() << pdfboxValidator.arguments().join(' ') << " in directory " << pdfboxValidator.workingDirectory() << ": " << pdfboxValidatorStandardError;
            pdfboxValidatorStandardOutput.prepend(QString(QStringLiteral("<error exitcode=\"%1\" exceededtimelimit=\"%2\" timelimitsec=\"%3\" />\n")).arg(pdfboxValidatorExitCode).arg(pdfboxValidatorTimeExceeded ? QStringLiteral("yes") : QStringLiteral("no")).arg(pdfboxValidatorTimeLimit / 1000));
        }

        if (pdfboxValidatorTimeExceeded)
            pdfboxValidator.kill();
    }

    if (doRunValidators && threeHeightsPDFValidatorStartedRun) {
        static const int threeHeightsPDFValidatorTimeLimit = twentyMinutesInMillisec;
        const bool threeHeightsPDFValidatorTimeExceeded = !threeHeightsPDFValidatorProcess.waitForFinished(threeHeightsPDFValidatorTimeLimit);
        if (threeHeightsPDFValidatorTimeExceeded)
            qWarning() << "Waiting for 3-Heights PDF Validator Shell failed or exceeded time limit (" << (threeHeightsPDFValidatorTimeLimit / 1000) << "s) for file " << filename << " and " << threeHeightsPDFValidatorProcess.program() << threeHeightsPDFValidatorProcess.arguments().join(' ') << " in directory " << threeHeightsPDFValidatorProcess.workingDirectory();
        threeHeightsPDFValidatorExitCode = threeHeightsPDFValidatorProcess.exitCode();
        threeHeightsPDFValidatorStandardOutput = QString::fromUtf8(threeHeightsPDFValidatorStandardOutputData.constData()).trimmed();
        threeHeightsPDFValidatorStandardError = QString::fromUtf8(threeHeightsPDFValidatorStandardErrorData.constData()).trimmed();

        if (threeHeightsPDFValidatorTimeExceeded || (threeHeightsPDFValidatorExitCode != 0 && threeHeightsPDFValidatorExitCode != 4) || threeHeightsPDFValidatorStandardOutput.isEmpty()) {
            qWarning() << "Execution of 3-Heights PDF Validator Shell failed for file " << filename << " and " << threeHeightsPDFValidatorProcess.program() << threeHeightsPDFValidatorProcess.arguments().join(' ') << " in directory " << threeHeightsPDFValidatorProcess.workingDirectory() << ": " << threeHeightsPDFValidatorStandardError;
            threeHeightsPDFValidatorStandardOutput.prepend(QString(QStringLiteral("<error exitcode=\"%1\" exceededtimelimit=\"%2\" timelimitsec=\"%3\" />\n")).arg(threeHeightsPDFValidatorExitCode).arg(threeHeightsPDFValidatorTimeExceeded ? QStringLiteral("yes") : QStringLiteral("no")).arg(threeHeightsPDFValidatorTimeLimit / 1000));
        }

        if (threeHeightsPDFValidatorTimeExceeded)
            threeHeightsPDFValidatorProcess.kill();
    }

    if (callasPdfAPilotStartedRun2) {
        static const int callasPdfAPilotTimeLimit = twentyMinutesInMillisec;
        const bool callasPdfAPilotTimeExceeded = !callasPdfAPilot.waitForFinished(callasPdfAPilotTimeLimit);
        if (callasPdfAPilotTimeExceeded)
            qWarning() << "Waiting for callas PDF/A Pilot failed or exceeded time limit (" << (callasPdfAPilotTimeLimit / 1000) << "s) for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory();
        callasPdfAPilotExitCode = callasPdfAPilot.exitCode();
        callasPdfAPilotStandardOutput = callasPdfAPilotStandardOutput + QStringLiteral("\n") + QString::fromUtf8(callasPdfAPilotStandardOutputData.constData()).trimmed();
        callasPdfAPilotStandardError = callasPdfAPilotStandardError + QStringLiteral("\n") + QString::fromUtf8(callasPdfAPilotStandardErrorData.constData()).trimmed();
        if (!callasPdfAPilotTimeExceeded && callasPdfAPilotExitCode == 0) {
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
        } else {
            qWarning() << "Execution of callas PDF/A Pilot failed for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory() << ": " << callasPdfAPilotStandardError;
            callasPdfAPilotStandardOutput.prepend(QString(QStringLiteral("<error exitcode=\"%1\" exceededtimelimit=\"%2\" timelimitsec=\"%3\" />\n")).arg(callasPdfAPilotExitCode).arg(callasPdfAPilotTimeExceeded ? QStringLiteral("yes") : QStringLiteral("no")).arg(callasPdfAPilotTimeLimit / 1000));
        }

        if (callasPdfAPilotTimeExceeded)
            callasPdfAPilot.kill();
    }

    if (doRunValidators && qoppaJPDFPreflightStarted) {
        static const int qoppaJPDFPreflightTimeLimit = sixtyMinutesInMillisec;
        const bool qoppaJPDFPreflightTimeExceeded = !qoppaJPDFPreflightProcess.waitForFinished(qoppaJPDFPreflightTimeLimit);
        if (qoppaJPDFPreflightTimeExceeded)
            qWarning() << "Waiting for Qoppa jPDFPreflight failed or exceeded time limit (" << (qoppaJPDFPreflightTimeLimit / 1000) << "s) for file " << filename << " and " << qoppaJPDFPreflightProcess.program() << qoppaJPDFPreflightProcess.arguments().join(' ') << " in directory " << qoppaJPDFPreflightProcess.workingDirectory();
        qoppaJPDFPreflightExitCode = qoppaJPDFPreflightProcess.exitCode();
        qoppaJPDFPreflightStandardOutput = QString::fromUtf8(qoppaJPDFPreflightStandardOutputData.constData()).trimmed();
        qoppaJPDFPreflightStandardError = QString::fromUtf8(qoppaJPDFPreflightStandardErrorData.constData()).trimmed();

        if (qoppaJPDFPreflightTimeExceeded || qoppaJPDFPreflightExitCode != 0 || qoppaJPDFPreflightStandardOutput.isEmpty()) {
            qWarning() << "Execution of Qoppa jPDFPreflight failed for file " << filename << " and " << qoppaJPDFPreflightProcess.program() << qoppaJPDFPreflightProcess.arguments().join(' ') << " in directory " << qoppaJPDFPreflightProcess.workingDirectory() << ": " << qoppaJPDFPreflightStandardError;
            qoppaJPDFPreflightStandardOutput.prepend(QString(QStringLiteral("<error exitcode=\"%1\" exceededtimelimit=\"%2\" timelimitsec=\"%3\" />\n")).arg(qoppaJPDFPreflightExitCode).arg(qoppaJPDFPreflightTimeExceeded ? QStringLiteral("yes") : QStringLiteral("no")).arg(qoppaJPDFPreflightTimeLimit / 1000));
        }

        if (qoppaJPDFPreflightTimeExceeded)
            qoppaJPDFPreflightProcess.kill();
    }

    const qint64 externalProgramsEndTime = QDateTime::currentMSecsSinceEpoch();

    if (doRunValidators && qoppaJPDFPreflightExitCode > INT_MIN) {
        const int p1 = qoppaJPDFPreflightStandardOutput.indexOf(QStringLiteral("<qoppapdfpreflight"));
        const int p2 = qoppaJPDFPreflightStandardOutput.indexOf(QStringLiteral("</qoppapdfpreflight>"), p1 + 1);
        if (p1 >= 0 && p2 > p1)
            metaText.append(qoppaJPDFPreflightStandardOutput.mid(p1, p2 - p1 + 20).replace(QStringLiteral("<qoppapdfpreflight "), QString(QStringLiteral("<qoppapdfpreflight exitcode=\"%1\" flavor=\"%2\" ")).arg(qoppaJPDFPreflightExitCode).arg(qoppaJPDFPreflightFlavor)) + QStringLiteral("\n"));
        else {
            qWarning() << "Missing expected XML output from Qoppa jPDFPreflight for file " << filename << " and " << qoppaJPDFPreflightProcess.program() << qoppaJPDFPreflightProcess.arguments().join(' ') << " in directory " << qoppaJPDFPreflightProcess.workingDirectory() << ": " << qoppaJPDFPreflightStandardError;
            metaText.append(QString(QStringLiteral("<qoppapdfpreflight exitcode=\"%1\" pdfa1b=\"no\" flavor=\"%3\"><error>Missing expected XML output</error><details>%2</details></qoppapdfpreflight>\n")).arg(qoppaJPDFPreflightExitCode).arg(DocScan::xmlify(qoppaJPDFPreflightStandardError), qoppaJPDFPreflightFlavor));
        }
    } else
        metaText.append(QStringLiteral("<qoppapdfpreflight><info>Qoppa not configured to run</info></qoppapdfpreflight>\n"));

    if (doRunValidators && jhoveExitCode > INT_MIN) {
        /// insert data from jHove
        metaText.append(QString(QStringLiteral("<jhove exitcode=\"%1\" wellformed=\"%2\" valid=\"%3\" pdf=\"%4\"")).arg(QString::number(jhoveExitCode), jhovePDFWellformed ? QStringLiteral("yes") : QStringLiteral("no"), jhovePDFValid ? QStringLiteral("yes") : QStringLiteral("no"), jhoveIsPDF ? QStringLiteral("yes") : QStringLiteral("no")));
        if (jhovePDFversion.isEmpty() && jhovePDFprofile.isEmpty() && jhoveStandardOutput.isEmpty() && jhoveStandardError.isEmpty())
            metaText.append(QStringLiteral(" />\n"));
        else {
            metaText.append(QStringLiteral(">\n"));
            if (!jhovePDFversion.isEmpty())
                metaText.append(QString(QStringLiteral("<version>%1</version>\n")).arg(DocScan::xmlify(jhovePDFversion)));
            if (!jhovePDFprofile.isEmpty()) {
                const bool isPDFA1a = jhovePDFprofile.contains(QStringLiteral("ISO PDF/A-1, Level A"));
                const bool isPDFA1b = jhovePDFprofile.contains(QStringLiteral("ISO PDF/A-1, Level B"));
                metaText.append(QString(QStringLiteral("<profile linear=\"%2\" tagged=\"%3\" pdfa1a=\"%4\" pdfa1b=\"%5\" pdfx3=\"%6\">%1</profile>\n")).arg(DocScan::xmlify(jhovePDFprofile), jhovePDFprofile.contains(QStringLiteral("Linearized PDF")) ? QStringLiteral("yes") : QStringLiteral("no"), jhovePDFprofile.contains(QStringLiteral("Tagged PDF")) ? QStringLiteral("yes") : QStringLiteral("no"), isPDFA1a ? QStringLiteral("yes") : QStringLiteral("no"), isPDFA1b ? QStringLiteral("yes") : QStringLiteral("no"), jhovePDFprofile.contains(QStringLiteral("ISO PDF/X-3")) ? QStringLiteral("yes") : QStringLiteral("no")));
            }
            if (!jhoveStandardOutput.isEmpty())
                metaText.append(QString(QStringLiteral("<output>%1</output>\n")).arg(DocScan::xmlifyLines(jhoveStandardOutput)));
            if (!jhoveStandardError.isEmpty())
                metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlifyLines(jhoveStandardError)));
            metaText.append(QStringLiteral("</jhove>\n"));
        }
    } else if (doRunValidators && !jhoveShellscript.isEmpty())
        metaText.append(QStringLiteral("<jhove><error>jHove failed to start or was never started</error></jhove>\n"));
    else
        metaText.append(QStringLiteral("<jhove><info>jHove not configured to run</info></jhove>\n"));

    if (doRunValidators && veraPDFExitCode > INT_MIN) {
        /// insert XML data from veraPDF
        metaText.append(QString(QStringLiteral("<verapdf exitcode=\"%1\" filesize=\"%2\" pdfa1b=\"%3\" pdfa1a=\"%4\" flavor=\"%5\">\n")).arg(QString::number(veraPDFExitCode), QString::number(veraPDFfilesize), veraPDFIsPDFA1B ? QStringLiteral("yes") : QStringLiteral("no"), veraPDFIsPDFA1A ? QStringLiteral("yes") : QStringLiteral("no"), veraPDFvalidationFlavor));
        if (!veraPDFStandardOutput.isEmpty()) {
            /// Check for and omit XML header if it exists
            const int p = veraPDFStandardOutput.indexOf(QStringLiteral("?>"));
            metaText.append(p > 1 ? veraPDFStandardOutput.mid(veraPDFStandardOutput.indexOf(QStringLiteral("<"), p)) : veraPDFStandardOutput);
        }
        if (!veraPDFStandardError.isEmpty())
            metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlifyLines(veraPDFStandardError)));
        metaText.append(QStringLiteral("</verapdf>\n"));
    } else if (doRunValidators && !m_veraPDFcliTool.isEmpty())
        metaText.append(QStringLiteral("<verapdf><error>veraPDF failed to start or was never started</error></verapdf>\n"));
    else
        metaText.append(QStringLiteral("<verapdf><info>veraPDF not configured to run</info></verapdf>\n"));

    if (doRunValidators && threeHeightsPDFValidatorExitCode > INT_MIN) {
        QTextStream ts(&threeHeightsPDFValidatorStandardOutput);
        QString report;
        bool insideIssues = false;
        bool threeHeightsPDFValidatorPDFA1b = false, threeHeightsPDFValidatorPDFA1a = false;
        const QString toBeRemoved = QStringLiteral("\"") + filename + QStringLiteral("\", ");
        while (!ts.atEnd()) {
            const QString line = ts.readLine();
            if (line.startsWith(QStringLiteral("\"/"))) {
                /// A line reporting an issue
                if (!insideIssues) {
                    report += QString(QStringLiteral("\n<issues clvalue=\"%1\">\n")).arg(threeHeightsPDFValidatorCLValue);
                    insideIssues = true;
                }
                report += QStringLiteral("<issue>") + DocScan::xmlify(QString(line).trimmed().remove(toBeRemoved)) + QStringLiteral("</issue>\n");
            } else if (line.endsWith("conform to the PDF/A-1b standard.")) {
                /// Final statement on compliance PDF/A-1b
                threeHeightsPDFValidatorPDFA1b = !line.contains(QStringLiteral("does not conform"));
            } else if (line.endsWith("conform to the PDF/A-1a standard.")) {
                /// Final statement on compliance PDF/A-1a
                threeHeightsPDFValidatorPDFA1a = !line.contains(QStringLiteral("does not conform"));
            }
        }
        if (insideIssues) report += QStringLiteral("</issues>\n");

        metaText.append(QString(QStringLiteral("<threeheightspdfvalidator exitcode=\"%1\" pdfa1b=\"%2\" pdfa1a=\"%3\">")).arg(threeHeightsPDFValidatorExitCode).arg(threeHeightsPDFValidatorPDFA1b ? QStringLiteral("yes") : QStringLiteral("no")).arg(threeHeightsPDFValidatorPDFA1a ? QStringLiteral("yes") : QStringLiteral("no")) + report + QStringLiteral("</threeheightspdfvalidator>\n"));
    } else
        metaText.append(QStringLiteral("<threeheightspdfvalidator><info>3-Heights PDF Validator Shell not configured to run</info></threeheightspdfvalidator>\n"));

    if (doRunValidators && pdfboxValidatorExitCode > INT_MIN) {
        /// insert result from Apache's PDFBox
        metaText.append(QString(QStringLiteral("<pdfboxvalidator exitcode=\"%1\" pdfa1b=\"%2\">\n")).arg(QString::number(pdfboxValidatorExitCode), pdfboxValidatorValidPdf ? QStringLiteral("yes") : QStringLiteral("no")));
        if (!pdfboxValidatorStandardOutput.isEmpty()) {
            QTextStream ts(&pdfboxValidatorStandardOutput);
            QString buffer;
            while (!ts.atEnd() && buffer.length() < 16384)
                buffer.append(ts.readLine(1024));
            if (!buffer.startsWith(QChar('<')) || !buffer.endsWith(QChar('>'))) {
                /// Output does not look like XML output or is capped.
                /// Treat like plain text.
                metaText.append(DocScan::xmlifyLines(buffer));
            } else
                metaText.append(buffer.replace(QStringLiteral("><error"), QStringLiteral(">\n<error"))); ///< keep output as it is, but insert line breaks before beginning of 'error' tag
            if (!ts.atEnd())
                metaText.append(QStringLiteral("\n<!--  PDFBox Validator output too long  -->"));
        }
        if (!pdfboxValidatorStandardError.isEmpty())
            metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlifyLines(pdfboxValidatorStandardError)));
        metaText.append(QStringLiteral("</pdfboxvalidator>\n"));
    } else if (doRunValidators && !m_pdfboxValidatorJavaClass.isEmpty())
        metaText.append(QStringLiteral("<pdfboxvalidator><error>pdfbox Validator failed to start or was never started</error></pdfboxvalidator>\n"));
    else
        metaText.append(QStringLiteral("<pdfboxvalidator><info>pdfbox Validator not configured to run</info></pdfboxvalidator>\n"));

    if (doRunValidators && callasPdfAPilotExitCode > INT_MIN) {
        const bool isPDFA1a = callasPdfAPilotPDFA1letter == 'a' && callasPdfAPilotCountErrors == 0 && callasPdfAPilotCountWarnings == 0;
        const bool isPDFA1b = callasPdfAPilotPDFA1letter == 'b' && callasPdfAPilotCountErrors == 0 && callasPdfAPilotCountWarnings == 0;
        metaText.append(QString(QStringLiteral("<callaspdfapilot exitcode=\"%1\" pdfa1b=\"%2\" pdfa1a=\"%3\">\n")).arg(QString::number(callasPdfAPilotExitCode), isPDFA1b ? QStringLiteral("yes") : QStringLiteral("no"), isPDFA1a ? QStringLiteral("yes") : QStringLiteral("no")));
        if (!callasPdfAPilotStandardOutput.isEmpty())
            metaText.append(DocScan::xmlifyLines(callasPdfAPilotStandardOutput));
        if (!callasPdfAPilotStandardError.isEmpty())
            metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlifyLines(callasPdfAPilotStandardError)));
        metaText.append(QStringLiteral("</callaspdfapilot>"));
    } else if (doRunValidators && !m_callasPdfAPilotCLI.isEmpty())
        metaText.append(QStringLiteral("<callaspdfapilot><error>callas PDF/A Pilot failed to start or was never started</error></callaspdfapilot>\n"));
    else
        metaText.append(QStringLiteral("<callaspdfapilot><info>callas PDF/A Pilot not configured to run</info></callaspdfapilot>\n"));

    /// file information including size
    const QFileInfo fi = QFileInfo(filename);
    metaText.append(QString(QStringLiteral("<file size=\"%1\" />\n")).arg(fi.size()));

    if (!metaText.isEmpty()) {
        metaText.squeeze();
        logText.append(QStringLiteral("<meta>\n")).append(metaText).append(QStringLiteral("</meta>\n"));
    }
    const qint64 endTime = QDateTime::currentMSecsSinceEpoch();

    logText.prepend(QString(QStringLiteral("<fileanalysis filename=\"%1\" status=\"ok\" time=\"%2\" external_time=\"%3\">\n")).arg(DocScan::xmlify(filename), QString::number(endTime - startTime), QString::number(externalProgramsEndTime - startTime)));
    logText += QStringLiteral("</fileanalysis>\n");

    if (adobePreflightReportAnalysisOk || popplerWrapperOk || jhoveIsPDF || pdfboxValidatorValidPdf)
        /// At least one tool thought the file was ok
        emit analysisReport(objectName(), logText);
    else
        /// No tool could handle this file, so give error message
        emit analysisReport(objectName(), QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" external_time=\"%2\"><meta><file size=\"%3\" /></meta></fileanalysis>\n")).arg(filename, QString::number(externalProgramsEndTime - startTime)).arg(fi.size()));

    m_isAlive = false;
}

const QStringList FileAnalyzerPDF::blacklistedFileExtensions = QStringList() << QStringLiteral(".pbm") << QStringLiteral(".ppm") << QStringLiteral(".ccitt") << QStringLiteral(".params") << QStringLiteral(".joboptions");
