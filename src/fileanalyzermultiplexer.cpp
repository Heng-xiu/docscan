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
#include <QProcess>
#include <QFile>
#include <QFileInfo>

#include "fileanalyzermultiplexer.h"
#include "general.h"

FileAnalyzerMultiplexer::FileAnalyzerMultiplexer(const QStringList &filters, QObject *parent)
    : FileAnalyzerAbstract(parent), m_filters(filters)
{
    qsrand(QTime::currentTime().msec());
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

void FileAnalyzerMultiplexer::setupJhove(const QString &shellscript, const QString &configFile)
{
    m_fileAnalyzerPDF.setupJhove(shellscript, configFile);
}

void FileAnalyzerMultiplexer::uncompressAnalyzefile(const QString &filename, const QString &extensionWithDot, const QString &uncompressTool)
{
    const QFileInfo fi(filename);
    const QString tempFilename = QLatin1String("/tmp/") + QString::number(qrand()) + QLatin1String("-") + fi.fileName();
    QFile::copy(filename, tempFilename);
    QProcess uncompressProcess(this);
    const QStringList arguments = QStringList() << tempFilename;
    uncompressProcess.start(uncompressTool, arguments);
    if (uncompressProcess.waitForStarted(10000) && uncompressProcess.waitForFinished(60000)) {
        const QString uncompressedFile = tempFilename.left(tempFilename.length() - extensionWithDot.length());
        const QString logText = QString(QLatin1String("<uncompress tool=\"%1\">\n<origin>%2</origin>\n<via>%3</via>\n<destination>%4</destination>\n</uncompress>")).arg(DocScan::xmlify(uncompressTool)).arg(DocScan::xmlify(filename)).arg(DocScan::xmlify(tempFilename)).arg(DocScan::xmlify(uncompressedFile));
        emit analysisReport(logText);
        analyzeFile(uncompressedFile);
        QFile::remove(uncompressedFile);
    }
}

void FileAnalyzerMultiplexer::analyzeFile(const QString &filename)
{
    static const QRegExp odfExtension(QLatin1String(".od[pst]$"));
    static const QRegExp openXMLExtension(QLatin1String(".(doc|ppt|xls)x$"));
    static const QRegExp compoundBinaryExtension(QLatin1String(".(doc|ppt|xls)$"));

    qDebug() << "Analyzing file" << filename;

    if (filename.endsWith(QLatin1String(".xz"))) {
        uncompressAnalyzefile(filename, QLatin1String(".xz"), QLatin1String("unxz"));
    } else  if (filename.endsWith(QLatin1String(".gz"))) {
        uncompressAnalyzefile(filename, QLatin1String(".gz"), QLatin1String("gunzip"));
    } else  if (filename.endsWith(QLatin1String(".bz2"))) {
        uncompressAnalyzefile(filename, QLatin1String(".bz2"), QLatin1String("bunzip2"));
    } else  if (filename.endsWith(QLatin1String(".lzma"))) {
        uncompressAnalyzefile(filename, QLatin1String(".lzma"), QLatin1String("lzma"));
    } else if (filename.endsWith(".pdf")) {
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
            // if (FileAnalyzerCompoundBinary::isRTFfile(filename))
            //     m_fileAnalyzerRTF.analyzeFile(filename);
            // else
            m_fileAnalyzerCompoundBinary.analyzeFile(filename);
        } else
            qDebug() << "Skipping unmatched extension" << compoundBinaryExtension.cap(0);
    } else if (filename.endsWith(".rtf")) {
        if (m_filters.contains(QLatin1String("*.rtf")))
            m_fileAnalyzerRTF.analyzeFile(filename);
        else
            qDebug() << "Skipping unmatched extension \".rtf\"";
    } else
        qWarning() << "Unknown filename extension for file " << filename;
}
