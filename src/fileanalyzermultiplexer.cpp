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

#include "fileanalyzermultiplexer.h"

#include <QRegExp>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

#include "general.h"

FileAnalyzerMultiplexer::FileAnalyzerMultiplexer(const QStringList &filters, QObject *parent)
    : FileAnalyzerAbstract(parent), m_filters(filters)
{
    qsrand(QTime::currentTime().msec());
    connect(&m_fileAnalyzerODF, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerPDF, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
    connect(&m_fileAnalyzerOpenXML, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
#ifdef HAVE_WV2
    connect(&m_fileAnalyzerCompoundBinary, SIGNAL(analysisReport(QString)), this, SIGNAL(analysisReport(QString)));
#endif // HAVE_WV2
}

bool FileAnalyzerMultiplexer::isAlive()
{
    return m_fileAnalyzerODF.isAlive() || m_fileAnalyzerPDF.isAlive() || m_fileAnalyzerOpenXML.isAlive();
}

void FileAnalyzerMultiplexer::setupJhove(const QString &shellscript)
{
    m_fileAnalyzerPDF.setupJhove(shellscript);
}

void FileAnalyzerMultiplexer::setupVeraPDF(const QString &cliTool) {
    m_fileAnalyzerPDF.setupVeraPDF(cliTool);
}

void FileAnalyzerMultiplexer::setupPdfBoXValidator(const QString &pdfboxValidatorJavaClass) {
    m_fileAnalyzerPDF.setupPdfBoXValidator(pdfboxValidatorJavaClass);
}

void FileAnalyzerMultiplexer::setupCallasPdfAPilotCLI(const QString &callasPdfAPilotCLI) {
    m_fileAnalyzerPDF.setupCallasPdfAPilotCLI(callasPdfAPilotCLI);
}

void FileAnalyzerMultiplexer::uncompressAnalyzefile(const QString &filename, const QString &extensionWithDot, const QString &uncompressTool)
{
    /// Default prefix for temporary file is a large random number
    const QString randomPrefix = QString::number(qrand());
    /// Create QFileInfo object to extract the 'basename'
    const QFileInfo fi(filename.left(filename.length() - extensionWithDot.length()));
    /// Build temporary filename
    const QString randomTempFilename = QStringLiteral("/tmp/.docscan-") + randomPrefix + QStringLiteral("-") + fi.fileName();

    /// Keep track of time
    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    /// Keep track of success
    bool success = true;

    /// QFile objects for input and output
    QFile inputFile(filename), outputFile(randomTempFilename);
    QString md5prefix; ///< Recording MD5 checksums of compressed and uncompressed PDF data
    QCryptographicHash compressedMd5(QCryptographicHash::Md5), uncompressedMd5(QCryptographicHash::Md5);
    if (inputFile.open(QFile::ReadOnly) && outputFile.open(QFile::WriteOnly)) {
        static const qint64 buffer_size = 1 << 20; ///< 1 MB buffer size
        static char buffer[buffer_size];

        QProcess uncompressProcess(this);
        uncompressProcess.start(uncompressTool);
        qint64 size = 0;
        while ((size = inputFile.read(buffer, buffer_size)) > 0) {
            compressedMd5.addData(buffer, size); ///< Use read compressed data to compute MD5 sum
            uncompressProcess.write(buffer, size); ///< Pipe compressed data into uncompression process
            if (!uncompressProcess.waitForBytesWritten()) {
                success = false;
                break;
            };

            /// Read uncompressed data from uncompression process
            size = qMin(uncompressProcess.bytesAvailable(), buffer_size);
            if (size > 0) {
                size = uncompressProcess.read(buffer, size);
                uncompressedMd5.addData(buffer, size); ///< Use uncompressed data to compute MD5 sum
                if (outputFile.write(buffer, size) != size) {
                    success = false;
                    break;
                };
            }
        }
        uncompressProcess.closeWriteChannel();
        inputFile.close();
        success &= uncompressProcess.waitForBytesWritten();

        /// Process remaining uncompressed data after piping compressed data
        /// to uncompression process is done
        uncompressProcess.waitForReadyRead(500);
        size = qMin(uncompressProcess.bytesAvailable(), buffer_size);
        while (success && size > 0) {
            size = uncompressProcess.read(buffer, size);
            if (size <= 0 || outputFile.write(buffer, size) != size) {
                success = false;
                break;
            };
            uncompressProcess.waitForReadyRead(500);
            size = qMin(uncompressProcess.bytesAvailable(), buffer_size);
        }
        outputFile.close();

        /// Retrieve MD5 sums of compressed and uncompressed PDF data
        md5prefix = QString::fromUtf8(compressedMd5.result().toHex()) + QChar('-') + QString::fromUtf8(uncompressedMd5.result().toHex());

        /// No data may have been written to stderr by uncompression process
        uncompressProcess.setReadChannel(QProcess::StandardError);
        success &= uncompressProcess.read(buffer, 1) < 1;
        /// Check if uncompression process exited ok.
        success &= uncompressProcess.state() == QProcess::NotRunning || uncompressProcess.waitForFinished();
        success &= uncompressProcess.exitStatus() == QProcess::NormalExit && uncompressProcess.exitCode() == 0;
    }

    QString uncompressedFilename = randomTempFilename;
    if (success && !md5prefix.isEmpty()) {
        /// If there is a valid MD5 sum prefix, rename temporary file accordingly
        uncompressedFilename = QStringLiteral("/tmp/.docscan-") + md5prefix + QStringLiteral("-") + fi.fileName();
        QFile::rename(randomTempFilename, uncompressedFilename);
    }

    const QString logText = QString(QStringLiteral("<uncompress status=\"%1\" tool=\"%2\" time=\"%3\">\n<origin md5sum=\"%5\">%4</origin>\n<destination md5sum=\"%7\">%6</destination>\n</uncompress>")).arg(success ? QStringLiteral("success") : QStringLiteral("error"), DocScan::xmlify(uncompressTool), QString::number(QDateTime::currentMSecsSinceEpoch() - startTime), DocScan::xmlify(filename), QString::fromUtf8(compressedMd5.result().toHex()), DocScan::xmlify(uncompressedFilename), QString::fromUtf8(uncompressedMd5.result().toHex()));
    emit analysisReport(logText);
    analyzeFile(uncompressedFilename);
    QFile::remove(uncompressedFilename); ///< Remove uncompressed file after analysis
}

void FileAnalyzerMultiplexer::analyzeFile(const QString &filename)
{
    static const QRegExp odfExtension(QStringLiteral("[.]od[pst]$"));
    static const QRegExp openXMLExtension(QStringLiteral("[.](doc|ppt|xls)x$"));
#ifdef HAVE_WV2
    static const QRegExp compoundBinaryExtension(QStringLiteral("[.](doc|ppt|xls)$"));
#endif // HAVE_WV2

    qDebug() << "Analyzing file" << filename;

    if (filename.endsWith(QStringLiteral(".xz"))) {
        uncompressAnalyzefile(filename, QStringLiteral(".xz"), QStringLiteral("unxz"));
    } else if (filename.endsWith(QStringLiteral(".gz"))) {
        uncompressAnalyzefile(filename, QStringLiteral(".gz"), QStringLiteral("gunzip"));
    } else if (filename.endsWith(QStringLiteral(".bz2"))) {
        uncompressAnalyzefile(filename, QStringLiteral(".bz2"), QStringLiteral("bunzip2"));
    } else if (filename.endsWith(QStringLiteral(".lzma"))) {
        uncompressAnalyzefile(filename, QStringLiteral(".lzma"), QStringLiteral("unlzma"));
    } else if (filename.endsWith(QStringLiteral(".pdf"))) {
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
#ifdef HAVE_WV2
    } else if (compoundBinaryExtension.indexIn(filename) >= 0) {
        if (m_filters.contains(QChar('*') + compoundBinaryExtension.cap(0))) {
            m_fileAnalyzerCompoundBinary.analyzeFile(filename);
        } else
            qDebug() << "Skipping unmatched extension" << compoundBinaryExtension.cap(0);
#endif // HAVE_WV2
    } else
        qWarning() << "Unknown filename extension for file " << filename;
}
