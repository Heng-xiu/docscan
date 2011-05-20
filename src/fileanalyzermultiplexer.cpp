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

#include <QRegExp>

#include "fileanalyzermultiplexer.h"

FileAnalyzerMultiplexer::FileAnalyzerMultiplexer(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
    connect(&m_fileAnalyzerODF, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerPDF, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerOpenXML, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerMicrosoftBinary, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
}

bool FileAnalyzerMultiplexer::isAlive() {
    return m_fileAnalyzerODF.isAlive() || m_fileAnalyzerPDF.isAlive() || m_fileAnalyzerOpenXML.isAlive();
}

void FileAnalyzerMultiplexer::analyzeFile(const QString &filename) {
    static QRegExp odfExtension(".od[pst]$");
    static QRegExp openXMLExtension(".(doc|ppt|xls)x$");
    static QRegExp microsoftBinaryExtension(".(doc|ppt|xls)x$");

    if (filename.endsWith(".pdf"))
        m_fileAnalyzerPDF.analyzeFile(filename);
    else if (filename.indexOf(odfExtension) >= 0)
        m_fileAnalyzerODF.analyzeFile(filename);
    else if (filename.indexOf(openXMLExtension) >= 0)
        m_fileAnalyzerOpenXML.analyzeFile(filename);
    else if (filename.indexOf(microsoftBinaryExtension) >= 0)
        m_fileAnalyzerMicrosoftBinary.analyzeFile(filename);
}
