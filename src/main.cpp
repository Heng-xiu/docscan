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
#include <QTextStream>
#include <QDebug>
#include <QThreadPool>

#include "networkaccessmanager.h"
#include "searchenginegoogle.h"
#include "searchenginebing.h"
#include "searchenginespringerlink.h"
#include "fakedownloader.h"
#include "urldownloader.h"
// #include "cachedfilefinder.h"
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
QString jhoveShellscript, jhoveConfigFile;
bool jhoveVerbose;
FileAnalyzerAbstract::TextExtraction textExtraction;

bool evaluateConfigfile(const QString &filename)
{
    QFile configFile(filename);
    if (configFile.open(QFile::ReadOnly)) {
        QTextStream ts(&configFile);
        ts.setCodec("utf-8");
        QString line(QString::null);
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

                if (key == "textextraction") {
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
                } else if (key == "requiredcontent") {
                    requiredContent = QRegExp(value);
                    qDebug() << "requiredContent =" << requiredContent.pattern();
                } else if (key == "jhove") {
                    const QStringList param = value.split(QChar('|'));
                    if (param.count() >= 2) {
                        jhoveShellscript = param[0];
                        jhoveConfigFile = param[1];
                        jhoveVerbose = param.count() >= 3 && (param[2].compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 || param[2].compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0);
                        qDebug() << "jhoveShellscript =" << jhoveShellscript;
                        qDebug() << "jhoveConfigFile =" << jhoveConfigFile;
                        qDebug() << "jhoveVerbose =" << jhoveVerbose;
                    }
                } else if (key == "webcrawler:starturl") {
                    startUrl = QUrl(value);
                    qDebug() << "webcrawler:startUrl =" << startUrl.toString();
                } else if (key == "filter") {
                    qDebug() << "filter =" << value;
                    filter = value.split(QChar('|'), QString::SkipEmptyParts);
                } else if (key == "springerlinkcategory") {
                    springerLinkCategory = value;
                    qDebug() << "springerlinkcategory =" << springerLinkCategory;
                } else if (key == "springerlinkcontenttype") {
                    springerLinkContentType = value;
                    qDebug() << "springerlinkcontenttype =" << springerLinkContentType;
                } else if (key == "springerlinksubject") {
                    springerLinkSubject = value;
                    qDebug() << "springerlinksubject =" << springerLinkSubject;
                } else if (key == "springerlinkyear") {
                    bool ok = false;
                    springerLinkYear = value.toInt(&ok);
                    if (!ok) springerLinkYear = SearchEngineSpringerLink::AllYears;
                    qDebug() << "springerlinkyear =" << (springerLinkYear == SearchEngineSpringerLink::AllYears ? QStringLiteral("No Year") : QString::number(springerLinkYear));
                } else if (key == "webcrawler:maxvisitedpages") {
                    bool ok = false;
                    webcrawlermaxvisitedpages = value.toInt(&ok);
                    if (!ok) webcrawlermaxvisitedpages = 1024;
                    qDebug() << "webcrawler:maxvisitedpages =" << webcrawlermaxvisitedpages;
                } else if (key == "webcrawler" && finder == NULL) {
                    qDebug() << "webcrawler =" << value << "using filter" << filter;
                    finder = new WebCrawler(netAccMan, filter, value, startUrl.isEmpty() ? QUrl(value) : startUrl, requiredContent, webcrawlermaxvisitedpages == 0 ? qMin(qMax(numHits * filter.count() * 256, 256), 4096) : webcrawlermaxvisitedpages);
                } else if (key == "searchenginegoogle" && finder == NULL) {
                    qDebug() << "searchenginegoogle =" << value;
                    finder = new SearchEngineGoogle(netAccMan, value);
                } else if (key == "searchenginebing" && finder == NULL) {
                    qDebug() << "searchenginebing =" << value;
                    finder = new SearchEngineBing(netAccMan, value);
                } else if (key == "searchenginespringerlink" && finder == NULL) {
                    qDebug() << "searchenginespringerlink =" << value;
                    finder = new SearchEngineSpringerLink(netAccMan, value, springerLinkCategory, springerLinkContentType, springerLinkSubject, springerLinkYear);
                } else if (key == "filesystemscan" && finder == NULL) {
                    qDebug() << "filesystemscan =" << value;
                    finder = new FileSystemScan(filter, value);
                } else if (key == "filefinderlist" && finder == NULL) {
                    qDebug() << "filefinderlist =" << value;
                    finder = new FileFinderList(value);
                } else if (key == "fromlogfilefilefinder" && finder == NULL) {
                    qDebug() << "fromlogfilefilefinder =" << value;
                    finder = new FromLogFileFileFinder(value, filter);
                } else if (key == "fromlogfiledownloader" && downloader == NULL) {
                    qDebug() << "fromlogfiledownloader =" << value;
                    downloader = new FromLogFileDownloader(value, filter);
                } else if (key == "urldownloader" && downloader == NULL) {
                    if (numHits > 0) {
                        qDebug() << "urldownloader =" << value << "   numHits=" << numHits;
                        downloader = new UrlDownloader(netAccMan, value, numHits);
                    } else {
                        qDebug() << "urldownloader =" << value;
                        downloader = new UrlDownloader(netAccMan, value);
                    }
                } else if (key == "fakedownloader" && downloader == NULL) {
                    qDebug() << "fakedownloader";
                    downloader = new FakeDownloader(netAccMan);
                } else if (key == "logcollector" && logCollector == NULL) {
                    qDebug() << "logcollector =" << value;
                    QFile *logOutput = new QFile(value);
                    logOutput->open(QFile::WriteOnly);
                    logCollector = new LogCollector(logOutput);
                } else if (key == "finder:numhits") {
                    bool ok = false;
                    numHits = value.toInt(&ok);
                    if (!ok) numHits = 10;
                    qDebug() << "finder:numhits =" << numHits;
                } else if (key == "fileanalyzer") {
                    if (value.contains("multiplexer")) {
                        fileAnalyzer = new FileAnalyzerMultiplexer(filter);
                        qDebug() << "fileanalyzer = FileAnalyzerMultiplexer";
                    } else if (value.contains("odf")) {
                        fileAnalyzer = new FileAnalyzerODF();
                        qDebug() << "fileanalyzer = FileAnalyzerODF";
                    } else if (value.contains("openxml")) {
                        fileAnalyzer = new FileAnalyzerOpenXML();
                        qDebug() << "fileanalyzer = FileAnalyzerOpenXML";
                    } else if (value.contains("pdf")) {
                        fileAnalyzer = new FileAnalyzerPDF();
                        qDebug() << "fileanalyzer = FileAnalyzerPDF";
                    } else if (value.contains("compoundbinary")) {
                        fileAnalyzer = new FileAnalyzerCompoundBinary();
                        qDebug() << "fileanalyzer = FileAnalyzerCompoundBinary";
                    } else
                        fileAnalyzer = NULL;
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
    fileAnalyzer = NULL;
    logCollector = NULL;
    downloader = NULL;
    finder = NULL;
    numHits = 0;
    webcrawlermaxvisitedpages = 0;
    textExtraction = FileAnalyzerAbstract::teNone;

    if (argc != 2) {
        fprintf(stderr, "Require single configuration file as parameter\n");
        return 1;
    } else if (!evaluateConfigfile(QString::fromUtf8(argv[argc - 1]))) {
        fprintf(stderr, "Evaluation of configuration file failed\n");
        return 1;
    } else if (logCollector == NULL) {
        fprintf(stderr, "Failed to instanciate log collector\n");
        return 1;
    } else if (numHits <= 0) {
        fprintf(stderr, "Number of expected hits is not positive\n");
        return 1;
    } else {
        WatchDog watchDog;
        if (fileAnalyzer != NULL) {
            watchDog.addWatchable(fileAnalyzer);
            fileAnalyzer->setTextExtraction(textExtraction);
        }
        if (downloader != NULL) watchDog.addWatchable(downloader);
        if (finder != NULL) watchDog.addWatchable(finder);
        watchDog.addWatchable(logCollector);

        if (!jhoveShellscript.isEmpty() && !jhoveConfigFile.isEmpty()) {
            FileAnalyzerPDF *fileAnalyzerPDF = qobject_cast<FileAnalyzerPDF *>(fileAnalyzer);
            if (fileAnalyzerPDF != NULL)
                fileAnalyzerPDF->setupJhove(jhoveShellscript, jhoveConfigFile, jhoveVerbose);
            else {
                FileAnalyzerMultiplexer *fileAnalyzerMultiplexer = qobject_cast<FileAnalyzerMultiplexer *>(fileAnalyzer);
                if (fileAnalyzerMultiplexer != NULL)
                    fileAnalyzerMultiplexer->setupJhove(jhoveShellscript, jhoveConfigFile, jhoveVerbose);
            }
        }

        if (downloader != NULL && finder != NULL) QObject::connect(finder, SIGNAL(foundUrl(QUrl)), downloader, SLOT(download(QUrl)));
        if (downloader != NULL && fileAnalyzer != NULL) QObject::connect(downloader, SIGNAL(downloaded(QString)), fileAnalyzer, SLOT(analyzeFile(QString)));
        QObject::connect(&watchDog, SIGNAL(quit()), &a, SLOT(quit()));
        if (downloader != NULL) QObject::connect(downloader, SIGNAL(report(QString)), logCollector, SLOT(receiveLog(const QString &)));
        if (fileAnalyzer != NULL) QObject::connect(fileAnalyzer, SIGNAL(analysisReport(QString)), logCollector, SLOT(receiveLog(const QString &)));
        if (finder != NULL) QObject::connect(finder, SIGNAL(report(QString)), logCollector, SLOT(receiveLog(const QString &)));
        if (downloader != NULL) QObject::connect(&watchDog, SIGNAL(firstWarning()), downloader, SLOT(finalReport()));
        QObject::connect(&watchDog, SIGNAL(lastWarning()), logCollector, SLOT(close()));

        if (finder != NULL) finder->startSearch(numHits);

        qDebug() << "activeThreadCount" << QThreadPool::globalInstance()->activeThreadCount() << "   maxThreadCount" << QThreadPool::globalInstance()->maxThreadCount();

        return a.exec();
    }
}
