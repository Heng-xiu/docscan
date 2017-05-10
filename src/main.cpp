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
#include "fakedownloader.h"
#include "urldownloader.h"
#include "filesystemscan.h"
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
int numHits, webcrawlermaxvisitedpages;
QString jhoveShellscript;
QString veraPDFcliTool;
QString pdfboxValidatorJavaClass;
QString callasPdfAPilotCLI;
FileAnalyzerAbstract::TextExtraction textExtraction;

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
                } else if (key == QStringLiteral("requiredcontent")) {
                    requiredContent = QRegExp(value);
                    qDebug() << "requiredContent =" << requiredContent.pattern();
                } else if (key == QStringLiteral("jhove")) {
                    jhoveShellscript = value;
                    qDebug() << "jhoveShellscript =" << jhoveShellscript;
                    const QFileInfo script(jhoveShellscript);
                    if (jhoveShellscript.isEmpty() || !script.exists() || !script.isExecutable())
                        qCritical() << "Value for jhoveShellscript does not refer to an existing, executable script or program";
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
                        qCritical() << "Value for pdfboxValidatorJavaClass does not refer to an non-existing xor executable file";
                } else if (key == QStringLiteral("callaspdfapilot")) {
                    callasPdfAPilotCLI = value;
                    qDebug() << "callaspdfapilot = " << callasPdfAPilotCLI;
                    const QFileInfo program(callasPdfAPilotCLI);
                    if (callasPdfAPilotCLI.isEmpty() || !program.exists() || !program.isExecutable())
                        qCritical() << "Value for callaspdfapilot does not refer to an existing, executable script or program";
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
                    if (numHits > 0) {
                        qDebug() << "urldownloader =" << value << "   numHits=" << numHits;
                        downloader = new UrlDownloader(netAccMan, value, numHits);
                    } else {
                        qDebug() << "urldownloader =" << value;
                        downloader = new UrlDownloader(netAccMan, value);
                    }
                } else if (key == QStringLiteral("fakedownloader") && downloader == nullptr) {
                    qDebug() << "fakedownloader";
                    downloader = new FakeDownloader(netAccMan);
                } else if (key == QStringLiteral("logcollector") && logCollector == nullptr) {
                    qDebug() << "logcollector =" << value;
                    QFile *logOutput = new QFile(value);
                    logOutput->open(QFile::WriteOnly);
                    logCollector = new LogCollector(logOutput);
                } else if (key == QStringLiteral("finder:numhits")) {
                    bool ok = false;
                    numHits = value.toInt(&ok);
                    if (!ok) numHits = 10;
                    qDebug() << "finder:numhits =" << numHits;
                } else if (key == QStringLiteral("fileanalyzer")) {
                    if (value.contains(QStringLiteral("multiplexer"))) {
                        if (filter.isEmpty())
                            qWarning() << "Attempting to create a FileAnalyzerMultiplexer with empty filter";
                        fileAnalyzer = new FileAnalyzerMultiplexer(filter);
                        qDebug() << "fileanalyzer = FileAnalyzerMultiplexer";
                    } else if (value.contains(QStringLiteral("odf"))) {
                        fileAnalyzer = new FileAnalyzerODF();
                        qDebug() << "fileanalyzer = FileAnalyzerODF";
                    } else if (value.contains(QStringLiteral("openxml"))) {
                        fileAnalyzer = new FileAnalyzerOpenXML();
                        qDebug() << "fileanalyzer = FileAnalyzerOpenXML";
                    } else if (value.contains(QStringLiteral("pdf"))) {
                        fileAnalyzer = new FileAnalyzerPDF();
                        qDebug() << "fileanalyzer = FileAnalyzerPDF";
#ifdef HAVE_WV2
                    } else if (value.contains(QStringLiteral("compoundbinary"))) {
                        fileAnalyzer = new FileAnalyzerCompoundBinary();
                        qDebug() << "fileanalyzer = FileAnalyzerCompoundBinary";
#endif // HAVE_WV2
                    } else
                        fileAnalyzer = nullptr;
                } else {
                    qDebug() << "UNKNOWN CONFIG:" << key << "=" << value;
                }
            } else {
                configFile.close();
                return false;
            }
        }
        configFile.close();
    } else
        return false;

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
    numHits = 0;
    webcrawlermaxvisitedpages = 0;
    textExtraction = FileAnalyzerAbstract::teNone;

    if (argc != 2) {
        fprintf(stderr, "Require single configuration file as parameter\n");
        return 1;
    } else if (!evaluateConfigfile(QString::fromUtf8(argv[argc - 1]))) {
        fprintf(stderr, "Evaluation of configuration file failed\n");
        return 1;
    } else if (logCollector == nullptr) {
        fprintf(stderr, "Failed to instanciate log collector\n");
        return 1;
    } else if (numHits <= 0) {
        fprintf(stderr, "Number of expected hits is not positive\n");
        return 1;
    } else {
        WatchDog watchDog;
        if (fileAnalyzer != nullptr) {
            watchDog.addWatchable(fileAnalyzer);
            fileAnalyzer->setTextExtraction(textExtraction);
        }
        if (downloader != nullptr) watchDog.addWatchable(downloader);
        if (finder != nullptr) watchDog.addWatchable(finder);
        watchDog.addWatchable(logCollector);

        if (!jhoveShellscript.isEmpty()) {
            FileAnalyzerPDF *fileAnalyzerPDF = qobject_cast<FileAnalyzerPDF *>(fileAnalyzer);
            if (fileAnalyzerPDF != nullptr) {
                fileAnalyzerPDF->setupJhove(jhoveShellscript);
            } else {
                FileAnalyzerMultiplexer *fileAnalyzerMultiplexer = qobject_cast<FileAnalyzerMultiplexer *>(fileAnalyzer);
                if (fileAnalyzerMultiplexer != nullptr) {
                    fileAnalyzerMultiplexer->setupJhove(jhoveShellscript);
                }
            }
        }

        if (!veraPDFcliTool.isEmpty()) {
            FileAnalyzerPDF *fileAnalyzerPDF = qobject_cast<FileAnalyzerPDF *>(fileAnalyzer);
            if (fileAnalyzerPDF != nullptr) {
                fileAnalyzerPDF->setupVeraPDF(veraPDFcliTool);
            } else {
                FileAnalyzerMultiplexer *fileAnalyzerMultiplexer = qobject_cast<FileAnalyzerMultiplexer *>(fileAnalyzer);
                if (fileAnalyzerMultiplexer != nullptr) {
                    fileAnalyzerMultiplexer->setupVeraPDF(veraPDFcliTool);
                }
            }
        }

        if (!pdfboxValidatorJavaClass.isEmpty()) {
            FileAnalyzerPDF *fileAnalyzerPDF = qobject_cast<FileAnalyzerPDF *>(fileAnalyzer);
            if (fileAnalyzerPDF != nullptr) {
                fileAnalyzerPDF->setupPdfBoXValidator(pdfboxValidatorJavaClass);
            } else {
                FileAnalyzerMultiplexer *fileAnalyzerMultiplexer = qobject_cast<FileAnalyzerMultiplexer *>(fileAnalyzer);
                if (fileAnalyzerMultiplexer != nullptr) {
                    fileAnalyzerMultiplexer->setupPdfBoXValidator(pdfboxValidatorJavaClass);
                }
            }
        }

        if (!callasPdfAPilotCLI.isEmpty()) {
            FileAnalyzerPDF *fileAnalyzerPDF = qobject_cast<FileAnalyzerPDF *>(fileAnalyzer);
            if (fileAnalyzerPDF != nullptr) {
                fileAnalyzerPDF->setupCallasPdfAPilotCLI(callasPdfAPilotCLI);
            } else {
                FileAnalyzerMultiplexer *fileAnalyzerMultiplexer = qobject_cast<FileAnalyzerMultiplexer *>(fileAnalyzer);
                if (fileAnalyzerMultiplexer != nullptr) {
                    fileAnalyzerMultiplexer->setupCallasPdfAPilotCLI(callasPdfAPilotCLI);
                }
            }
        }

        if (downloader != nullptr && finder != nullptr) QObject::connect(finder, SIGNAL(foundUrl(QUrl)), downloader, SLOT(download(QUrl)));
        if (downloader != nullptr && fileAnalyzer != nullptr) QObject::connect(downloader, SIGNAL(downloaded(QString)), fileAnalyzer, SLOT(analyzeFile(QString)));
        QObject::connect(&watchDog, SIGNAL(quit()), &a, SLOT(quit()));
        if (downloader != nullptr) QObject::connect(downloader, SIGNAL(report(QString)), logCollector, SLOT(receiveLog(const QString &)));
        if (fileAnalyzer != nullptr) QObject::connect(fileAnalyzer, SIGNAL(analysisReport(QString)), logCollector, SLOT(receiveLog(const QString &)));
        if (finder != nullptr) QObject::connect(finder, SIGNAL(report(QString)), logCollector, SLOT(receiveLog(const QString &)));
        if (downloader != nullptr) QObject::connect(&watchDog, SIGNAL(firstWarning()), downloader, SLOT(finalReport()));
        QObject::connect(&watchDog, SIGNAL(lastWarning()), logCollector, SLOT(close()));

        if (finder != nullptr) finder->startSearch(numHits);

        qDebug() << "activeThreadCount" << QThreadPool::globalInstance()->activeThreadCount() << "   maxThreadCount" << QThreadPool::globalInstance()->maxThreadCount();

        return a.exec();
    }
}
