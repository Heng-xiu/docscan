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
#include <QNetworkAccessManager>

#include "searchenginegoogle.h"
#include "downloader.h"
#include "fileanalyzerodf.h"
#include "fileanalyzerpdf.h"
#include "fileanalyzeropenxml.h"
#include "watchdog.h"
#include "logcollector.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QNetworkAccessManager netAccMan;
    SearchEngineGoogle searchEngine(&netAccMan, QLatin1String("filetype:docx openoffice | libreoffice"));
    Downloader downloader(&netAccMan, QLatin1String("/tmp/test/%{h:4}/%{h}_%{s}"));
    FileAnalyzerOpenXML fileAnalyzer;

    WatchDog watchDog;
    QFile output("/tmp/log.txt");
    output.open(QFile::WriteOnly);
    LogCollector logCollector(output);

    watchDog.addWatchable(&fileAnalyzer);
    watchDog.addWatchable(&downloader);
    watchDog.addWatchable(&searchEngine);
    watchDog.addWatchable(&logCollector);

    QObject::connect(&searchEngine, SIGNAL(foundUrl(QUrl)), &downloader, SLOT(download(QUrl)));
    QObject::connect(&downloader, SIGNAL(downloaded(QString)), &fileAnalyzer, SLOT(analyzeFile(QString)));
    QObject::connect(&watchDog, SIGNAL(aboutToQuit()), &logCollector, SLOT(writeOut()));
    QObject::connect(&watchDog, SIGNAL(quit()), &a, SLOT(quit()));
    QObject::connect(&downloader, SIGNAL(downloadReport(QString)), &logCollector, SLOT(receiveLog(QString)));
    QObject::connect(&fileAnalyzer, SIGNAL(analysisReport(QString)), &logCollector, SLOT(receiveLog(QString)));
    QObject::connect(&searchEngine, SIGNAL(report(QString)), &logCollector, SLOT(receiveLog(QString)));

    searchEngine.startSearch(9);

    return a.exec();
}
