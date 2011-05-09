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

#include "searchenginebing.h"
#include "downloader.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QFile logfile("/tmp/log.csv");
    logfile.open(QIODevice::WriteOnly);
    QTextStream ts(&logfile);
    SearchEngineBing bing("filetype:pdf site:fi");
    Downloader downloader("/tmp/test/%{h}_%{s}", ts);
    bing.startSearch(5);
    QObject::connect(&bing, SIGNAL(foundUrl(QUrl)), &downloader, SLOT(download(QUrl)));

    return a.exec();
}
