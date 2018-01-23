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

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QThreadPool>

#include "networkaccessmanager.h"
#include "searchenginegoogle.h"
#include "searchenginebing.h"
#include "searchenginespringerlink.h"
#include "downloader.h"
#include "fakedownloader.h"
#include "urldownloader.h"
#include "filesystemscan.h"
#include "directorymonitor.h"
#include "fileanalyzermultiplexer.h"
#include "watchdog.h"
#include "webcrawler.h"
#include "logcollector.h"
#include "fromlogfile.h"
#include "filefinderlist.h"

NetworkAccessManager *netAccMan;
QStringList filter;
FileFinder *finder;
Downloader *downloader;
LogCollector *logCollector;
FileAnalyzerAbstract *fileAnalyzer;
static const int defaultNumHits = 25000;
int numHits, webcrawlermaxvisitedpages;
QString jhoveShellscript;
QString dpfmangerJFXjar;
QString veraPDFcliTool;
QString pdfboxValidatorJavaClass;
QString callasPdfAPilotCLI;
QString adobePreflightReportDirectory;
QString qoppaJPDFPreflightDirectory;
QString threeHeightsValidatorShellCLI, threeHeightsValidatorLicenseKey;
bool downgradeToPDFA1b;
FileAnalyzerAbstract::TextExtraction textExtraction;
bool enableEmbeddedFilesAnalysis;

bool evaluateConfigfile(const QString &filename)
{
    QFile configFile(filename);
    if (configFile.open(QFile::ReadOnly)) {
        QTextStream ts(&configFile);
        ts.setCodec("utf-8");
        QString line;
        QUrl startUrl;
        QRegExp requiredContent;
        QString springerLinkCategory = SearchEngineSpringerLink::AllCategories;
        QString springerLinkContentType = SearchEngineSpringerLink::AllContentTypes;
        QString springerLinkSubject = SearchEngineSpringerLink::AllSubjects;
        int springerLinkYear = SearchEngineSpringerLink::AllYears;

        while (!(line = ts.readLine()).isNull()) {
            if (line.length() == 0 || line[0] == '#') continue;
            int i = line.indexOf('=');
            if (i > 1) {
                QString key = line.left(i).simplified().toLower();
                QString value = line.mid(i + 1).simplified();

                if (key == QStringLiteral("textextraction")) {
                    if (value.compare(QStringLiteral("none"), Qt::CaseInsensitive) == 0)
                        textExtraction = FileAnalyzerAbstract::teNone;
                    else if (value.compare(QStringLiteral("length"), Qt::CaseInsensitive) == 0)
                        textExtraction = FileAnalyzerAbstract::teLength;
                    else if (value.compare(QStringLiteral("fulltext"), Qt::CaseInsensitive) == 0)
                        textExtraction = FileAnalyzerAbstract::teFullText;
                    else if (value.compare(QStringLiteral("aspell"), Qt::CaseInsensitive) == 0)
                        textExtraction = FileAnalyzerAbstract::teAspell;
                    else
                        qWarning() << "Invalid value for \"textExtraction\":" << value;
                } else if (key == QStringLiteral("embeddedfilesanalysis")) {
                    enableEmbeddedFilesAnalysis = value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 || value.compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0;
                } else if (key == QStringLiteral("downgradetopdfa1b")) {
                    downgradeToPDFA1b = value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 || value.compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0;
                } else if (key == QStringLiteral("requiredcontent")) {
                    requiredContent = QRegExp(value);
                    qDebug() << "requiredContent =" << requiredContent.pattern();
                } else if (key == QStringLiteral("jhove")) {
                    jhoveShellscript = value;
                    qDebug() << "jhoveShellscript =" << jhoveShellscript;
                    const QFileInfo script(jhoveShellscript);
                    if (jhoveShellscript.isEmpty() || !script.exists() || !script.isExecutable())
                        qCritical() << "Value for jhoveShellscript does not refer to an existing, executable script or program";
                } else if (key == QStringLiteral("dpfmanagerjar")) {
                    dpfmangerJFXjar = value;
                    qDebug() << "dpfmangerJFXjar =" << dpfmangerJFXjar;
                    const QFileInfo javaClass(dpfmangerJFXjar);
                    if (dpfmangerJFXjar.isEmpty() || !javaClass.exists() || javaClass.isExecutable())
                        qCritical() << "Value for dpfmangerJFXjar does refer to an non-existing xor executable file: " << dpfmangerJFXjar;
                } else if (key == QStringLiteral("verapdf")) {
                    veraPDFcliTool = value;
                    qDebug() << "veraPDFcliTool = " << veraPDFcliTool;
                    const QFileInfo script(veraPDFcliTool);
                    if (veraPDFcliTool.isEmpty() || !script.exists() || !script.isExecutable())
                        qCritical() << "Value for verapdf does not refer to an existing, executable script or program";
                } else if (key == QStringLiteral("pdfboxvalidator")) {
                    pdfboxValidatorJavaClass = value;
                    qDebug() << "pdfboxvalidator = " << pdfboxValidatorJavaClass;
                    const QFileInfo javaClass(pdfboxValidatorJavaClass);
                    if (pdfboxValidatorJavaClass.isEmpty() || !javaClass.exists() || javaClass.isExecutable())
                        qCritical() << "Value for pdfboxValidatorJavaClass does not refer to an non-existing xor executable file: " << pdfboxValidatorJavaClass;
                } else if (key == QStringLiteral("callaspdfapilot")) {
                    callasPdfAPilotCLI = value;
                    qDebug() << "callaspdfapilot = " << callasPdfAPilotCLI;
                    const QFileInfo program(callasPdfAPilotCLI);
                    if (callasPdfAPilotCLI.isEmpty() || !program.exists() || !program.isExecutable())
                        qCritical() << "Value for callaspdfapilot does not refer to an existing, executable script or program";
                } else if (key == QStringLiteral("adobepreflightreportdirectory")) {
                    adobePreflightReportDirectory = value;
                    qDebug() << "adobepreflightreportdirectory = " << adobePreflightReportDirectory;
                    const QFileInfo directory(adobePreflightReportDirectory);
                    if (!directory.exists() || !directory.isReadable())
                        qCritical() << "Value for adobepreflightreportdirectory does not refer to an existing and readable directory";
                } else if (key == QStringLiteral("qoppajpdfpreflightdirectory")) {
                    qoppaJPDFPreflightDirectory = value;
                    qDebug() << "qoppajpdfpreflightdirectory = " << qoppaJPDFPreflightDirectory;
                    const QFileInfo directory(qoppaJPDFPreflightDirectory);
                    if (!directory.exists() || !directory.isReadable())
                        qCritical() << "Value for qoppajpdfpreflightdirectory does not refer to an existing and readable directory";
                } else if (key == QStringLiteral("threeheightsvalidatorshell")) {
                    const QStringList arguments = value.split(QLatin1Char('|'));
                    if (arguments.size() == 2) {
                        threeHeightsValidatorShellCLI = arguments[0];
                        threeHeightsValidatorLicenseKey = arguments[1];
                        qDebug() << "threeHeightsValidatorShellCLI = " << threeHeightsValidatorShellCLI;
                        qDebug() << "threeHeightsValidatorLicenseKey = " << threeHeightsValidatorLicenseKey;
                        const QFileInfo threeHeightsShellBinary(threeHeightsValidatorShellCLI);
                        if (!threeHeightsShellBinary.exists() || !threeHeightsShellBinary.isExecutable())
                            qCritical() << "Value for threeHeightsValidatorShell does not refer to an existing and executable binary";
                    } else
                        qCritical() << "Require two arguments separated by '|' for key 'threeHeightsValidatorShell': binary's path and license key";
                } else if (key == QStringLiteral("webcrawler:starturl")) {
                    startUrl = QUrl(value);
                    qDebug() << "webcrawler:startUrl =" << startUrl.toString();
                } else if (key == QStringLiteral("filter")) {
                    qDebug() << "filter =" << value;
                    filter = value.split(QChar('|'), QString::SkipEmptyParts);
                } else if (key == QStringLiteral("springerlinkcategory")) {
                    springerLinkCategory = value;
                    qDebug() << "springerlinkcategory =" << springerLinkCategory;
                } else if (key == QStringLiteral("springerlinkcontenttype")) {
                    springerLinkContentType = value;
                    qDebug() << "springerlinkcontenttype =" << springerLinkContentType;
                } else if (key == QStringLiteral("springerlinksubject")) {
                    springerLinkSubject = value;
                    qDebug() << "springerlinksubject =" << springerLinkSubject;
                } else if (key == QStringLiteral("springerlinkyear")) {
                    bool ok = false;
                    springerLinkYear = value.toInt(&ok);
                    if (!ok) springerLinkYear = SearchEngineSpringerLink::AllYears;
                    qDebug() << "springerlinkyear =" << (springerLinkYear == SearchEngineSpringerLink::AllYears ? QStringLiteral("No Year") : QString::number(springerLinkYear));
                } else if (key == QStringLiteral("webcrawler:maxvisitedpages")) {
                    bool ok = false;
                    webcrawlermaxvisitedpages = value.toInt(&ok);
                    if (!ok) webcrawlermaxvisitedpages = 1024;
                    qDebug() << "webcrawler:maxvisitedpages =" << webcrawlermaxvisitedpages;
                } else if (key == QStringLiteral("webcrawler") && finder == nullptr) {
                    qDebug() << "webcrawler =" << value << "using filter" << filter;
                    finder = new WebCrawler(netAccMan, filter, value, startUrl.isEmpty() ? QUrl(value) : startUrl, requiredContent, webcrawlermaxvisitedpages == 0 ? qMin(qMax(numHits * filter.count() * 256, 256), 4096) : webcrawlermaxvisitedpages);
                } else if (key == QStringLiteral("searchenginegoogle") && finder == nullptr) {
                    qDebug() << "searchenginegoogle =" << value;
                    finder = new SearchEngineGoogle(netAccMan, value);
                } else if (key == QStringLiteral("searchenginebing") && finder == nullptr) {
                    qDebug() << "searchenginebing =" << value;
                    finder = new SearchEngineBing(netAccMan, value);
                } else if (key == QStringLiteral("searchenginespringerlink") && finder == nullptr) {
                    qDebug() << "searchenginespringerlink =" << value;
                    finder = new SearchEngineSpringerLink(netAccMan, value, springerLinkCategory, springerLinkContentType, springerLinkSubject, springerLinkYear);
                } else if (key == QStringLiteral("filesystemscan") && finder == nullptr) {
                    qDebug() << "filesystemscan =" << value;
                    finder = new FileSystemScan(filter, value);
                } else if (key == QStringLiteral("directorymonitor") && finder == nullptr) {
                    const QStringList arguments = value.split(QChar(','));
                    if (arguments.count() == 2 && !arguments[0].isEmpty() && !arguments[1].isEmpty()) {
                        bool ok = false;
                        int timeout = arguments[1].toInt(&ok);
                        if (ok) {
                            qDebug() << "directorymonitor =" << arguments[0] << "  timeout=" << timeout;
                            finder = new DirectoryMonitor(timeout, filter, arguments[0]);
                        } else
                            qWarning() << "Is not a number: " << arguments[1];
                    } else
                        qWarning() << "Invalid argument for 'directorymonitor': " << value;
                } else if (key == QStringLiteral("filefinderlist") && finder == nullptr) {
                    qDebug() << "filefinderlist =" << value;
                    finder = new FileFinderList(value);
                } else if (key == QStringLiteral("fromlogfilefilefinder") && finder == nullptr) {
                    qDebug() << "fromlogfilefilefinder =" << value;
                    finder = new FromLogFileFileFinder(value, filter);
                } else if (key == QStringLiteral("fromlogfiledownloader") && downloader == nullptr) {
                    qDebug() << "fromlogfiledownloader =" << value;
                    downloader = new FromLogFileDownloader(value, filter);
                } else if (key == QStringLiteral("urldownloader") && downloader == nullptr) {
                    qDebug() << "urldownloader =" << value << "   numHits=" << numHits;
                    downloader = new UrlDownloader(netAccMan, value, numHits);
                } else if (key == QStringLiteral("fakedownloader") && downloader == nullptr) {
                    /// Deprecated setting key. If no other downloader is configured,
                    /// a FakeDownloader instance will be automatically created and used.
                } else if (key == QStringLiteral("logcollector") && logCollector == nullptr) {
                    qDebug() << "logcollector =" << value;
                    QFile *logOutput = new QFile(value);
                    logOutput->open(QFile::WriteOnly);
                    logCollector = new LogCollector(logOutput);
                } else if (key == QStringLiteral("finder:numhits")) {
                    bool ok = false;
                    numHits = value.toInt(&ok);
                    if (!ok || numHits <= 0) numHits = defaultNumHits;
                    qDebug() << "finder:numhits =" << numHits;
                } else if (key == QStringLiteral("fileanalyzer")) {
                    if (value.contains(QStringLiteral("multiplexer"))) {
                        if (filter.isEmpty())
                            qWarning() << "Attempting to create a FileAnalyzerMultiplexer with empty filter";
                        fileAnalyzer = new FileAnalyzerMultiplexer(filter);
                        qDebug() << "fileanalyzer = FileAnalyzerMultiplexer";
#ifdef HAVE_QUAZIP5
                    } else if (value.contains(QStringLiteral("odf"))) {
                        fileAnalyzer = new FileAnalyzerODF();
                        qDebug() << "fileanalyzer = FileAnalyzerODF";
                    } else if (value.contains(QStringLiteral("openxml"))) {
                        fileAnalyzer = new FileAnalyzerOpenXML();
                        qDebug() << "fileanalyzer = FileAnalyzerOpenXML";
                    } else if (value.contains(QStringLiteral("zip"))) {
                        fileAnalyzer = new FileAnalyzerZIP();
                        qDebug() << "fileanalyzer = FileAnalyzerZIP";
#endif // HAVE_QUAZIP5
                    } else if (value.contains(QStringLiteral("pdf"))) {
                        fileAnalyzer = new FileAnalyzerPDF();
                        qDebug() << "fileanalyzer = FileAnalyzerPDF";
#ifdef HAVE_WV2
                    } else if (value.contains(QStringLiteral("compoundbinary"))) {
                        fileAnalyzer = new FileAnalyzerCompoundBinary();
                        qDebug() << "fileanalyzer = FileAnalyzerCompoundBinary";
#endif // HAVE_WV2
                    } else if (value.contains(QStringLiteral("jpeg"))) {
                        fileAnalyzer = new FileAnalyzerJPEG();
                        qDebug() << "fileanalyzer = FileAnalyzerJPEG";
                    } else if (value.contains(QStringLiteral("jp2"))) {
                        fileAnalyzer = new FileAnalyzerJP2();
                        qDebug() << "fileanalyzer = FileAnalyzerJP2";
                    } else if (value.contains(QStringLiteral("tiff"))) {
                        fileAnalyzer = new FileAnalyzerTIFF();
                        qDebug() << "fileanalyzer = FileAnalyzerTIFF";
                    } else
                        fileAnalyzer = nullptr;
                } else {
                    qDebug() << "UNKNOWN CONFIG:" << key << "=" << value;
                }
            } else {
                qWarning() << "Invalid line: " << line;
                configFile.close();
                return false;
            }
        }
        configFile.close();
    } else {
        qWarning() << "Failed to open " << filename;
        return false;
    }

    return true;
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "%s\n", localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stdout, "Info: %s\n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication a(argc, argv);

    netAccMan = new NetworkAccessManager(&a);
    fileAnalyzer = nullptr;
    logCollector = nullptr;
    downloader = nullptr;
    finder = nullptr;
    numHits = defaultNumHits;
    webcrawlermaxvisitedpages = 0;
    textExtraction = FileAnalyzerAbstract::teNone;
    enableEmbeddedFilesAnalysis = false;
    downgradeToPDFA1b = false;

    if (argc != 2) {
        fprintf(stderr, "Require single configuration file as parameter\n");
        return 1;
    } else if (!evaluateConfigfile(QString::fromUtf8(argv[argc - 1]))) {
        fprintf(stderr, "Evaluation of configuration file failed\n");
        return 1;
    } else if (logCollector == nullptr) {
        fprintf(stderr, "Failed to instanciate log collector\n");
        return 1;
    } else {
        if (downloader == nullptr) {
            /// No downloader defined in configuration file?
            /// Fall back to use 'FakeDownloader' that can only process
            /// (i.e. hand through) local files
            downloader = new FakeDownloader(netAccMan);
        }

        FileAnalyzerMultiplexer *fileAnalyzerMultiplexer = qobject_cast<FileAnalyzerMultiplexer *>(fileAnalyzer);
        if (fileAnalyzerMultiplexer == nullptr) {
            /// There is always need for a multiplexer. So, if the user did configure
            /// to use a specialized type of file analyzer, still create a multiplexer.
            fileAnalyzerMultiplexer = new FileAnalyzerMultiplexer(FileAnalyzerMultiplexer::defaultFilters, &a);
            QObject::connect(fileAnalyzerMultiplexer, &FileAnalyzerAbstract::analysisReport, logCollector, &LogCollector::receiveLog);
        }

        WatchDog watchDog;
        if (fileAnalyzer != nullptr) {
            watchDog.addWatchable(fileAnalyzer);
            fileAnalyzer->setTextExtraction(textExtraction);
            fileAnalyzer->setAnalyzeEmbeddedFiles(enableEmbeddedFilesAnalysis);
        }
        if (downloader != nullptr) watchDog.addWatchable(downloader);
        if (finder != nullptr) watchDog.addWatchable(finder);
        watchDog.addWatchable(logCollector);

        if (downloader != nullptr && finder != nullptr) QObject::connect(finder, &FileFinder::foundUrl, downloader, &Downloader::download);
        if (downloader != nullptr && fileAnalyzer != nullptr) QObject::connect(downloader, static_cast<void(Downloader::*)(QString)>(&Downloader::downloaded), fileAnalyzer, &FileAnalyzerAbstract::analyzeFile);
        QObject::connect(&watchDog, &WatchDog::quit, &a, &QCoreApplication::quit);
        if (downloader != nullptr) QObject::connect(downloader, &Downloader::report, logCollector, &LogCollector::receiveLog);
        if (fileAnalyzer != nullptr) {
            QObject::connect(fileAnalyzer, &FileAnalyzerAbstract::analysisReport, logCollector, &LogCollector::receiveLog);
            QObject::connect(fileAnalyzer, &FileAnalyzerAbstract::foundEmbeddedFile, fileAnalyzerMultiplexer, &FileAnalyzerMultiplexer::analyzeTemporaryFile);
        }
        if (finder != nullptr) QObject::connect(finder, &FileFinder::report, logCollector, &LogCollector::receiveLog);
        if (downloader != nullptr) QObject::connect(&watchDog, &WatchDog::firstWarning, downloader, &Downloader::finalReport);
        QObject::connect(&watchDog, &WatchDog::lastWarning, logCollector, &LogCollector::close);

        FileAnalyzerPDF *fileAnalyzerPDF = qobject_cast<FileAnalyzerPDF *>(fileAnalyzer);

        if (!jhoveShellscript.isEmpty()) {
            if (fileAnalyzerPDF != nullptr)
                fileAnalyzerPDF->setupJhove(jhoveShellscript);
            else {
                FileAnalyzerJPEG *fileAnalyzerJPEG = qobject_cast<FileAnalyzerJPEG *>(fileAnalyzer);
                if (fileAnalyzerJPEG != nullptr)
                    fileAnalyzerJPEG->setupJhove(jhoveShellscript);
                else {
                    FileAnalyzerJP2 *fileAnalyzerJP2 = qobject_cast<FileAnalyzerJP2 *>(fileAnalyzer);
                    if (fileAnalyzerJP2 != nullptr)
                        fileAnalyzerJP2->setupJhove(jhoveShellscript);
                    else {
                        FileAnalyzerTIFF *fileAnalyzerTIFF = qobject_cast<FileAnalyzerTIFF *>(fileAnalyzer);
                        if (fileAnalyzerTIFF != nullptr)
                            fileAnalyzerTIFF->setupJhove(jhoveShellscript);
                    }
                }
            }
            if (fileAnalyzerMultiplexer != nullptr)
                fileAnalyzerMultiplexer->setupJhove(jhoveShellscript);
        }

        if (!dpfmangerJFXjar.isEmpty()) {
            FileAnalyzerTIFF *fileAnalyzerTIFF = qobject_cast<FileAnalyzerTIFF *>(fileAnalyzer);
            if (fileAnalyzerTIFF != nullptr)
                fileAnalyzerTIFF->setupDPFManager(dpfmangerJFXjar);
            if (fileAnalyzerMultiplexer != nullptr)
                fileAnalyzerMultiplexer->setupDPFManager(dpfmangerJFXjar);
        }

        if (!veraPDFcliTool.isEmpty()) {
            if (fileAnalyzerPDF != nullptr)
                fileAnalyzerPDF->setupVeraPDF(veraPDFcliTool);
            if (fileAnalyzerMultiplexer != nullptr)
                fileAnalyzerMultiplexer->setupVeraPDF(veraPDFcliTool);
        }

        if (!pdfboxValidatorJavaClass.isEmpty()) {
            if (fileAnalyzerPDF != nullptr)
                fileAnalyzerPDF->setupPdfBoXValidator(pdfboxValidatorJavaClass);
            if (fileAnalyzerMultiplexer != nullptr)
                fileAnalyzerMultiplexer->setupPdfBoXValidator(pdfboxValidatorJavaClass);
        }

        if (!callasPdfAPilotCLI.isEmpty()) {
            if (fileAnalyzerPDF != nullptr)
                fileAnalyzerPDF->setupCallasPdfAPilotCLI(callasPdfAPilotCLI);
            if (fileAnalyzerMultiplexer != nullptr)
                fileAnalyzerMultiplexer->setupCallasPdfAPilotCLI(callasPdfAPilotCLI);
        }

        if (!adobePreflightReportDirectory.isEmpty()) {
            if (fileAnalyzerPDF != nullptr)
                fileAnalyzerPDF->setAdobePreflightReportDirectory(adobePreflightReportDirectory);
            if (fileAnalyzerMultiplexer != nullptr)
                fileAnalyzerMultiplexer->setAdobePreflightReportDirectory(adobePreflightReportDirectory);
        }

        if (!qoppaJPDFPreflightDirectory.isEmpty()) {
            if (fileAnalyzerPDF != nullptr)
                fileAnalyzerPDF->setQoppaJPDFPreflightDirectory(qoppaJPDFPreflightDirectory);
            if (fileAnalyzerMultiplexer != nullptr)
                fileAnalyzerMultiplexer->setQoppaJPDFPreflightDirectory(qoppaJPDFPreflightDirectory);
        }

        if (!threeHeightsValidatorShellCLI.isEmpty() && !threeHeightsValidatorLicenseKey.isEmpty()) {
            if (fileAnalyzerPDF != nullptr)
                fileAnalyzerPDF->setupThreeHeightsValidatorShellCLI(threeHeightsValidatorShellCLI, threeHeightsValidatorLicenseKey);
            if (fileAnalyzerMultiplexer != nullptr)
                fileAnalyzerMultiplexer->setupThreeHeightsValidatorShellCLI(threeHeightsValidatorShellCLI, threeHeightsValidatorLicenseKey);
        }

        if (fileAnalyzerPDF != nullptr)
            fileAnalyzerPDF->setDowngradePDFAConformance(downgradeToPDFA1b);
        if (fileAnalyzerMultiplexer != nullptr)
            fileAnalyzerMultiplexer->setDowngradePDFAConformance(downgradeToPDFA1b);

        if (finder != nullptr) finder->startSearch(numHits);

        qDebug() << "activeThreadCount" << QThreadPool::globalInstance()->activeThreadCount() << "   maxThreadCount" << QThreadPool::globalInstance()->maxThreadCount();

        return a.exec();
    }
}
