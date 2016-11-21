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
}

bool FileAnalyzerMultiplexer::isAlive()
{
    return m_fileAnalyzerODF.isAlive() || m_fileAnalyzerPDF.isAlive() || m_fileAnalyzerOpenXML.isAlive();
}

void FileAnalyzerMultiplexer::setupJhove(const QString &shellscript, const QString &configFile, bool verbose)
{
    m_fileAnalyzerPDF.setupJhove(shellscript, configFile, verbose);
}

void FileAnalyzerMultiplexer::setupVeraPDF(const QString &cliTool) {
    m_fileAnalyzerPDF.setupVeraPDF(cliTool);
}

void FileAnalyzerMultiplexer::uncompressAnalyzefile(const QString &filename, const QString &extensionWithDot, const QString &uncompressTool)
{
    const QFileInfo fi(filename);
    const QString tempFilename = QStringLiteral("/tmp/") + QString::number(qrand()) + QStringLiteral("-") + fi.fileName();
    QFile::copy(filename, tempFilename);
    QProcess uncompressProcess(this);
    const QStringList arguments = QStringList() << tempFilename;
    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    uncompressProcess.start(uncompressTool, arguments);
    if (uncompressProcess.waitForStarted(10000) && uncompressProcess.waitForFinished(60000)) {
        const QString uncompressedFile = tempFilename.left(tempFilename.length() - extensionWithDot.length());
        bool uncompressSuccess = uncompressProcess.exitCode() == 0;
        const QString logText = QString(QStringLiteral("<uncompress status=\"%5\" tool=\"%1\" time=\"%5\">\n<origin>%2</origin>\n<via>%3</via>\n<destination>%4</destination>\n</uncompress>")).arg(DocScan::xmlify(uncompressTool)).arg(DocScan::xmlify(filename)).arg(DocScan::xmlify(tempFilename)).arg(DocScan::xmlify(uncompressedFile)).arg(uncompressSuccess ? QStringLiteral("success") : QStringLiteral("error")).arg(QDateTime::currentMSecsSinceEpoch() - startTime);
        emit analysisReport(logText);
        if (uncompressSuccess)
            analyzeFile(uncompressedFile);
        QFile::remove(uncompressedFile);
    }
    QFile::remove(tempFilename); /// in case temporary file was not removed/replaced by uncompress process
}

void FileAnalyzerMultiplexer::analyzeFile(const QString &filename)
{
    static const QRegExp odfExtension(QStringLiteral("[.]od[pst]$"));
    static const QRegExp openXMLExtension(QStringLiteral("[.](doc|ppt|xls)x$"));
    static const QRegExp compoundBinaryExtension(QStringLiteral("[.](doc|ppt|xls)$"));

    qDebug() << "Analyzing file" << filename;

    if (filename.endsWith(QStringLiteral(".xz"))) {
        uncompressAnalyzefile(filename, QStringLiteral(".xz"), QStringLiteral("unxz"));
    } else if (filename.endsWith(QStringLiteral(".gz"))) {
        uncompressAnalyzefile(filename, QStringLiteral(".gz"), QStringLiteral("gunzip"));
    } else if (filename.endsWith(QStringLiteral(".bz2"))) {
        uncompressAnalyzefile(filename, QStringLiteral(".bz2"), QStringLiteral("bunzip2"));
    } else if (filename.endsWith(QStringLiteral(".lzma"))) {
        uncompressAnalyzefile(filename, QStringLiteral(".lzma"), QStringLiteral("unlzma"));
    } else if (filename.endsWith(".pdf")) {
        if (m_filters.contains(QStringLiteral("*.pdf")))
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
            m_fileAnalyzerCompoundBinary.analyzeFile(filename);
        } else
            qDebug() << "Skipping unmatched extension" << compoundBinaryExtension.cap(0);
    } else
        qWarning() << "Unknown filename extension for file " << filename;
}
