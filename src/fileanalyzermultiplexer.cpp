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
#include <QDebug>

#include "fileanalyzermultiplexer.h"

FileAnalyzerMultiplexer::FileAnalyzerMultiplexer(const QStringList &filters, QObject *parent)
    : FileAnalyzerAbstract(parent), m_filters(filters)
{
    connect(&m_fileAnalyzerODF, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerPDF, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerOpenXML, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerCompoundBinary, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerRTF, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
}

bool FileAnalyzerMultiplexer::isAlive()
{
    return m_fileAnalyzerODF.isAlive() || m_fileAnalyzerPDF.isAlive() || m_fileAnalyzerOpenXML.isAlive();
}

void FileAnalyzerMultiplexer::analyzeFile(const QString &filename)
{
    static QRegExp odfExtension(".od[pst]$");
    static QRegExp openXMLExtension(".(doc|ppt|xls)x$");
    static QRegExp compoundBinaryExtension(".(doc|ppt|xls)$");

    qDebug() << "Analyzing file" << filename;

    if (filename.endsWith(".pdf")) {
        if (m_filters.contains(QLatin1String("*.pdf")))
            m_fileAnalyzerPDF.analyzeFile(filename);
        else
            qDebug() << "Skipping unmatched extension \".pdf\"";
    } else if (odfExtension.indexIn(filename) >= 0) {
        if (m_filters.contains(QChar('*') + odfExtension.cap(0)))
            m_fileAnalyzerODF.analyzeFile(filename);
        else
            qDebug() << "Skipping unmatched extension" << odfExtension.cap(0);
    } else if (openXMLExtension.indexIn(filename) >= 0) {
        if (m_filters.contains(QChar('*') + openXMLExtension.cap(0)))
            m_fileAnalyzerOpenXML.analyzeFile(filename);
        else
            qDebug() << "Skipping unmatched extension" << openXMLExtension.cap(0);
    } else if (compoundBinaryExtension.indexIn(filename) >= 0) {
        if (m_filters.contains(QChar('*') + compoundBinaryExtension.cap(0))) {
            if (FileAnalyzerCompoundBinary::isRTFfile(filename))
                m_fileAnalyzerRTF.analyzeFile(filename);
            else
                m_fileAnalyzerCompoundBinary.analyzeFile(filename);
        } else
            qDebug() << "Skipping unmatched extension" << compoundBinaryExtension.cap(0);
    } else if (filename.endsWith(".rtf")) {
        if (m_filters.contains(QLatin1String("*.rtf")))
            m_fileAnalyzerRTF.analyzeFile(filename);
        else
            qDebug() << "Skipping unmatched extension \".rtf\"";
    } else
        qWarning() << "Could not analyze file " << filename;
}
