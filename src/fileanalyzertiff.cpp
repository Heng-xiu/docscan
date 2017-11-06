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


    Copyright (2017) Thomas Fischer <thomas.fischer@his.se>, senior
    lecturer at University of Skövde, as part of the LIM-IT project.

 */

#include "fileanalyzertiff.h"

#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QTemporaryDir>

#include "general.h"

static const int oneMinuteInMillisec = 60000;
static const int twoMinutesInMillisec = oneMinuteInMillisec * 2;
static const int fourMinutesInMillisec = oneMinuteInMillisec * 4;
static const int sixMinutesInMillisec = oneMinuteInMillisec * 6;

FileAnalyzerTIFF::FileAnalyzerTIFF(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());
}

bool FileAnalyzerTIFF::isAlive()
{
    return false; // TODO
}

void FileAnalyzerTIFF::setupJhove(const QString &shellscript) {
    const QString report = JHoveWrapper::setupJhove(this, shellscript);
    if (!report.isEmpty()) {
        /// This was the first time 'setupJhove' inherited from JHoveWrapper was called
        /// and it gave us a report back. Propagate this report to the logging system
        emit analysisReport(QStringLiteral("jhovewrapper"), report);
    }
}

void FileAnalyzerTIFF::setupDPFManager(const QString &dpfmangerJFXjar_) {
    // TODO validity check
    dpfmangerJFXjar = dpfmangerJFXjar_;
}

void FileAnalyzerTIFF::analyzeFile(const QString &filename)
{
    /// External programs should be both CPU and I/O 'nice'
    static const QStringList defaultArgumentsForNice = QStringList() << QStringLiteral("-n") << QStringLiteral("17") << QStringLiteral("ionice") << QStringLiteral("-c") << QStringLiteral("3");

    // TODO code de-duplication with FileAnalyzerJPEG and FileAnalyzerJP2

    QProcess *jhoveProcess = launchJHove(this, JHoveTIFF, filename);
    QByteArray jhoveStandardOutputData, jhoveStandardErrorData;
    connect(jhoveProcess, &QProcess::readyReadStandardOutput, [jhoveProcess, &jhoveStandardOutputData]() {
        const QByteArray d(jhoveProcess->readAllStandardOutput());
        jhoveStandardOutputData.append(d);
    });
    connect(jhoveProcess, &QProcess::readyReadStandardError, [jhoveProcess, &jhoveStandardErrorData]() {
        const QByteArray d(jhoveProcess->readAllStandardError());
        jhoveStandardErrorData.append(d);
    });
    const bool jhoveStarted = jhoveProcess != nullptr && jhoveProcess->waitForStarted(oneMinuteInMillisec);
    if (jhoveProcess != nullptr && !jhoveStarted)
        qWarning() << "Failed to start jhove for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory();

    QProcess dpfManagerProcess(this);
    QByteArray dpfManagerStandardOutputData, dpfManagerStandardErrorData;
    connect(&dpfManagerProcess, &QProcess::readyReadStandardOutput, [&dpfManagerProcess, &dpfManagerStandardOutputData]() {
        const QByteArray d(dpfManagerProcess.readAllStandardOutput());
        dpfManagerStandardOutputData.append(d);
    });
    connect(&dpfManagerProcess, &QProcess::readyReadStandardError, [&dpfManagerProcess, &dpfManagerStandardErrorData]() {
        const QByteArray d(dpfManagerProcess.readAllStandardError());
        dpfManagerStandardErrorData.append(d);
    });
    QTemporaryDir dpfManagerTempDir;
    const QStringList dpfManagerArguments = QStringList(defaultArgumentsForNice) << QStringLiteral("java") << QStringLiteral("-Duser.home=") + dpfManagerTempDir.path() + QStringLiteral("/home") << QStringLiteral("-jar") << dpfmangerJFXjar << QStringLiteral("check") << QStringLiteral("-f")  << QStringLiteral("xml") << QStringLiteral("-o") << dpfManagerTempDir.path() + QStringLiteral("/output") << QStringLiteral("-r") << QStringLiteral("0") << filename;
    dpfManagerProcess.setWorkingDirectory(dpfManagerTempDir.path());
    dpfManagerProcess.start(QStringLiteral("/usr/bin/nice"), dpfManagerArguments, QIODevice::ReadOnly);
    const bool dpfManagerStarted = dpfManagerProcess.waitForStarted(oneMinuteInMillisec);

    int jhoveExitCode = INT_MIN;
    bool jhoveIsTIFF = false;
    bool jhoveIsWellformedAndValid = false;
    int jhoveImageWidth = INT_MIN, jhoveImageHeight = INT_MIN, jhoveFilesize = INT_MIN;
    QString jhoveErrorMessage;
    QDate jhoveCreationDate, jhoveModificationDate;
    QString jhoveStandardOutput;
    QString jhoveErrorOutput;
    if (jhoveStarted) {
        if (!jhoveProcess->waitForFinished(fourMinutesInMillisec))
            qWarning() << "Waiting for jHove failed or exceeded time limit for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory();
        jhoveExitCode = jhoveProcess->exitCode();
        jhoveStandardOutput = QString::fromUtf8(jhoveStandardOutputData);
        jhoveErrorOutput = QString::fromUtf8(jhoveStandardErrorData);
        if (jhoveExitCode == 0 && !jhoveStandardOutput.isEmpty()) {
            jhoveIsTIFF = jhoveStandardOutput.contains(QStringLiteral(" MIMEtype: image/tiff"));
            jhoveIsWellformedAndValid = jhoveStandardOutput.contains(QStringLiteral(" Status: Well-Formed and valid"));
            static const QRegularExpression imageSizeRegExp(QStringLiteral(" Image(Width|Height): ([1-9][0-9]*)"));
            QRegularExpressionMatchIterator itImageSize = imageSizeRegExp.globalMatch(jhoveStandardOutput);
            while (itImageSize.hasNext()) {
                const QRegularExpressionMatch match = itImageSize.next();
                bool ok = false;
                if (match.captured(1) == QStringLiteral("Width")) {
                    jhoveImageWidth = match.captured(2).toInt(&ok);
                    if (!ok) jhoveImageWidth = INT_MIN;
                } else if (match.captured(1) == QStringLiteral("Height")) {
                    jhoveImageHeight = match.captured(2).toInt(&ok);
                    if (!ok) jhoveImageHeight = INT_MIN;
                }
            }
            static const QRegularExpression fileSizeRegExp(QStringLiteral(" Size: ([1-9][0-9]*)"));
            const QRegularExpressionMatch fileSizeMatch = fileSizeRegExp.match(jhoveStandardOutput);
            if (fileSizeMatch.hasMatch()) {
                bool ok = false;
                jhoveFilesize = fileSizeMatch.captured(1).toInt(&ok);
                if (!ok) jhoveFilesize = INT_MIN;
            }
            static const QRegularExpression dateRegExp(QStringLiteral(" xmp:(Create|Modify)Date=\"((19[89]|20[012345])[0-9]-[01][0-9]-[0-3][0-9])"));
            QRegularExpressionMatchIterator itDate = dateRegExp.globalMatch(jhoveStandardOutput);
            while (itDate.hasNext()) {
                const QRegularExpressionMatch match = itDate.next();
                const QDate date = QDate::fromString(match.captured(2), QStringLiteral("yyyy-MM-dd"));
                if (date.isValid()) {
                    if (match.captured(1) == QStringLiteral("Create"))
                        jhoveCreationDate = date;
                    else if (match.captured(1) == QStringLiteral("Modify"))
                        jhoveModificationDate = date;
                }
            }
            static const QRegularExpression errorMessageRegExp(QStringLiteral(" ErrorMessage: (.*?)"));
            const QRegularExpressionMatch errorMessageMatch = errorMessageRegExp.match(jhoveStandardOutput);
            if (errorMessageMatch.hasMatch())
                jhoveErrorMessage = errorMessageMatch.captured(1);
        } else
            qWarning() << "Execution of jHove failed for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory() << ": " << jhoveErrorOutput;
    }

    QString dpfManagerResult;
    int dpfManagerExitCode = INT_MIN;
    QString dpfManagerStandardOutput, dpfManagerErrorOutput;
    if (dpfManagerStarted) {
        if (!dpfManagerProcess.waitForFinished(fourMinutesInMillisec))
            qWarning() << "Waiting for DPFManager failed or exceeded time limit for file " << filename << " and " << dpfManagerProcess.program() << dpfManagerProcess.arguments().join(' ') << " in directory " << dpfManagerProcess.workingDirectory();
        dpfManagerExitCode = dpfManagerProcess.exitCode();
        dpfManagerStandardOutput = QString::fromUtf8(dpfManagerStandardOutputData);
        dpfManagerErrorOutput = QString::fromUtf8(dpfManagerStandardErrorData);
        if (dpfManagerExitCode == 0 && !dpfManagerStandardOutput.isEmpty()) {
            const QDir outputDir(dpfManagerTempDir.path() + QStringLiteral("/output"), QStringLiteral("1-*.xml"), QDir::Name | QDir::DirsLast | QDir::IgnoreCase, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
            const QStringList fileList = outputDir.entryList(QStringList() << QStringLiteral("1-*.xml"), QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name | QDir::DirsLast | QDir::IgnoreCase);
            if (fileList.count() == 2) {
                const QString xmlLogfile = outputDir.path() + QDir::separator() + (fileList.first().contains(QStringLiteral(".mets.")) ? fileList.last() : fileList.first());
                QFile xmlFile(xmlLogfile);
                if (xmlFile.open(QFile::ReadOnly)) {
                    const QString xmlData = QString::fromUtf8(xmlFile.readAll()).remove(QRegExp(QStringLiteral("<\\?xml[^>]*>"))).remove(QRegExp(QStringLiteral("<tiff_structure.*</tiff_structure>")));
                    const bool isTiff = xmlData.contains(QStringLiteral("<ImageWidth"));
                    const bool tiffBaselineCore6 = xmlData.contains(QStringLiteral(" TIFF_Baseline_Core_6_0=\"true\""));
                    const bool tiffBaselineExtended6 = xmlData.contains(QStringLiteral(" TIFF_Baseline_Extended_6_0==\"true\""));
                    dpfManagerResult = QString(QStringLiteral("<dpfmanager exitcode=\"%1\" tiff=\"%2\" baseline=\"%3\" extended=\"%4\">\n")).arg(dpfManagerExitCode).arg(isTiff ? QStringLiteral("yes") : QStringLiteral("no"), tiffBaselineCore6 ? QStringLiteral("yes") : QStringLiteral("no"), tiffBaselineExtended6 ? QStringLiteral("yes") : QStringLiteral("no")) + xmlData + QStringLiteral("\n</dpfmanager>\n");
                    xmlFile.close();
                } else
                    qWarning() << "Execution of DPFManager failed for file " << filename << " and " <<  dpfManagerProcess.program() << dpfManagerProcess.arguments().join(' ') << " in directory " << dpfManagerProcess.workingDirectory() << ": Could not open file " << xmlLogfile;
            } else
                qWarning() << "Execution of DPFManager failed for file " << filename << " and " <<  dpfManagerProcess.program() << dpfManagerProcess.arguments().join(' ') << " in directory " << dpfManagerProcess.workingDirectory() << ": Expected 2 output XML file, got " << fileList.count();
        }
    }

    if (!jhoveStarted)
        emit analysisReport(objectName(), QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"jhove-not-started\" status=\"error\" />\n")).arg(DocScan::xmlify(filename)));
    else {
        QString report = QString(QStringLiteral("<fileanalysis filename=\"%1\" status=\"ok\">\n")).arg(DocScan::xmlify(filename));
        report.append(QString(QStringLiteral("<jhove exitcode=\"%1\" tiff=\"%2\" wellformedandvalid=\"%3\">\n")).arg(QString::number(jhoveExitCode), jhoveIsTIFF ? QStringLiteral("yes") : QStringLiteral("no"), jhoveIsWellformedAndValid ? QStringLiteral("yes") : QStringLiteral("no")));

        if (jhoveIsTIFF && jhoveImageWidth > INT_MIN && jhoveImageHeight > INT_MIN && jhoveFilesize > INT_MIN) {
            report.append(QStringLiteral("<meta>\n"));
            if (jhoveCreationDate.isValid())
                report.append(DocScan::formatDate(jhoveCreationDate, creationDate));
            if (jhoveModificationDate.isValid())
                report.append(DocScan::formatDate(jhoveModificationDate, modificationDate));
            report.append(QString(QStringLiteral("<rect width=\"%1\" height=\"%2\" />\n")).arg(jhoveImageWidth).arg(jhoveImageHeight));
            report.append(QString(QStringLiteral("<file size=\"%1\" />\n")).arg(jhoveFilesize));
            report.append(QStringLiteral("</meta>\n"));
        }

        if (!jhoveErrorMessage.isEmpty())
            report.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlifyLines(jhoveErrorMessage)));
        if (!jhoveErrorOutput.isEmpty())
            report.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlifyLines(jhoveErrorOutput)));

        report.append(QStringLiteral("</jhove>\n"));
        report.append(dpfManagerResult);
        report.append(QStringLiteral("</fileanalysis>"));
        emit analysisReport(objectName(), report);
    }
}
