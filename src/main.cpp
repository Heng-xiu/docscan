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
#include "urldownloader.h"
// #include "cachedfilefinder.h"
#include "filesystemscan.h"
#include "fileanalyzermultiplexer.h"
#include "watchdog.h"
#include "webcrawler.h"
#include "logcollector.h"
#include "fromlogfile.h"

NetworkAccessManager *netAccMan;
QStringList filter;
FileFinder *finder;
Downloader *downloader;
LogCollector *logCollector;
FileAnalyzerAbstract *fileAnalyzer;
int numHits, webcrawlermaxvisitedpages;
QString jhoveShellscript, jhoveConfigFile;

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

                if (key == "requiredcontent") {
                    requiredContent = QRegExp(value);
                    qDebug() << "requiredContent =" << requiredContent.pattern();
                } else if (key == "jhove") {
                    const QStringList param = value.split(QChar('|'));
                    if (param.count() == 2) {
                        jhoveShellscript = param[0];
                        jhoveConfigFile = param[1];
                        qDebug() << "jhoveShellscript =" << jhoveShellscript;
                        qDebug() << "jhoveConfigFile =" << jhoveConfigFile;
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
                    qDebug() << "springerlinkyear =" << (springerLinkYear == SearchEngineSpringerLink::AllYears ? QLatin1String("No Year") : QString::number(springerLinkYear));
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
                } else if (key == "fromlogfilefilefinder" && finder == NULL) {
                    qDebug() << "fromlogfilefilefinder =" << value;
                    finder = new FromLogFileFileFinder(value, filter);
                } else if (key == "fromlogfiledownloader" && downloader == NULL) {
                    qDebug() << "fromlogfiledownloader =" << value;
                    downloader = new FromLogFileDownloader(value, filter);
                } else if (key == "urldownloader" && downloader == NULL) {
                    qDebug() << "urldownloader =" << value;
                    downloader = new UrlDownloader(netAccMan, value);
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
                    } else if (value.contains("rtf")) {
                        fileAnalyzer = new FileAnalyzerRTF();
                        qDebug() << "fileanalyzer = FileAnalyzerRTF";
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

void myMessageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "%s\n", msg);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        break;
    }
}

int main(int argc, char *argv[])
{
    qInstallMsgHandler(myMessageOutput);
    QCoreApplication a(argc, argv);

    netAccMan = new NetworkAccessManager(&a);
    fileAnalyzer = NULL;
    logCollector = NULL;
    downloader = NULL;
    finder = NULL;
    numHits = 0;
    webcrawlermaxvisitedpages = 0;

    if (argc == 2 && evaluateConfigfile(QLatin1String(argv[argc - 1])) && logCollector != NULL && numHits > 0) {
        WatchDog watchDog;
        if (fileAnalyzer != NULL) watchDog.addWatchable(fileAnalyzer);
        if (downloader != NULL) watchDog.addWatchable(downloader);
        if (finder != NULL) watchDog.addWatchable(finder);
        watchDog.addWatchable(logCollector);

        if (!jhoveShellscript.isEmpty() && !jhoveConfigFile.isEmpty()) {
            FileAnalyzerPDF *fileAnalyzerPDF = qobject_cast<FileAnalyzerPDF *>(fileAnalyzer);
            if (fileAnalyzerPDF != NULL)
                fileAnalyzerPDF->setupJhove(jhoveShellscript, jhoveConfigFile);
            else {
                FileAnalyzerMultiplexer *fileAnalyzerMultiplexer = qobject_cast<FileAnalyzerMultiplexer *>(fileAnalyzer);
                if (fileAnalyzerMultiplexer != NULL)
                    fileAnalyzerMultiplexer->setupJhove(jhoveShellscript, jhoveConfigFile);
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
    } else {
        fprintf(stderr, "Require single configuration file as parameter\n");
        return 1;
    }
}
