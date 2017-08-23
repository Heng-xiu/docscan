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

#include "fileanalyzerjpeg.h"

#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

#include "general.h"

static const int oneMinuteInMillisec = 60000;
static const int twoMinutesInMillisec = oneMinuteInMillisec * 2;
static const int fourMinutesInMillisec = oneMinuteInMillisec * 4;
static const int sixMinutesInMillisec = oneMinuteInMillisec * 6;

FileAnalyzerJPEG::FileAnalyzerJPEG(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());
}

bool FileAnalyzerJPEG::isAlive()
{
    return false; // TODO
}

void FileAnalyzerJPEG::setupJhove(const QString &shellscript) {
    const QString report = JHoveWrapper::setupJhove(this, shellscript);
    if (!report.isEmpty()) {
        /// This was the first time 'setupJhove' inherited from JHoveWrapper was called
        /// and it gave us a report back. Propagate this report to the logging system
        emit analysisReport(QStringLiteral("jhovewrapper"), report);
    }
}

void FileAnalyzerJPEG::analyzeFile(const QString &filename)
{
    // TODO code de-duplication with FileAnalyzerJP2

    QProcess *jhoveProcess = launchJHove(this, JHoveJPEG, filename);
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

    // TODO add more tests there while JHove is running

    int jhoveExitCode = INT_MIN;
    bool jhoveIsJPEG = false;
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
        jhoveStandardOutput = QString::fromUtf8(jhoveStandardOutputData).replace(QLatin1Char('\n'), QStringLiteral("###"));
        jhoveErrorOutput = QString::fromUtf8(jhoveStandardErrorData).replace(QLatin1Char('\n'), QStringLiteral("###"));
        if (jhoveExitCode == 0 && !jhoveStandardOutput.isEmpty()) {
            jhoveIsJPEG = jhoveStandardOutput.contains(QStringLiteral(" MIMEtype: image/jpeg###"));
            jhoveIsWellformedAndValid = jhoveStandardOutput.contains(QStringLiteral(" Status: Well-Formed and valid###"));
            static const QRegularExpression imageSizeRegExp(QStringLiteral(" Image(Width|Height): ([1-9][0-9]*)###"));
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
            static const QRegularExpression fileSizeRegExp(QStringLiteral(" Size: ([1-9][0-9]*)###"));
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
            static const QRegularExpression errorMessageRegExp(QStringLiteral(" ErrorMessage: (.*?)###"));
            const QRegularExpressionMatch errorMessageMatch = errorMessageRegExp.match(jhoveStandardOutput);
            if (errorMessageMatch.hasMatch())
                jhoveErrorMessage = errorMessageMatch.captured(1);
        } else
            qWarning() << "Execution of jHove failed for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory() << ": " << jhoveErrorOutput;
    }

    if (!jhoveStarted)
        emit analysisReport(objectName(), QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"jhove-not-started\" status=\"error\" />\n")).arg(DocScan::xmlify(filename)));
    else {
        QString report = QString(QStringLiteral("<fileanalysis filename=\"%1\" status=\"ok\">\n")).arg(DocScan::xmlify(filename));
        report.append(QString(QStringLiteral("<jhove exitcode=\"%1\" jpeg=\"%2\" wellformedandvalid=\"%3\">\n")).arg(QString::number(jhoveExitCode), jhoveIsJPEG ? QStringLiteral("yes") : QStringLiteral("no"), jhoveIsWellformedAndValid ? QStringLiteral("yes") : QStringLiteral("no")));

        if (jhoveIsJPEG && jhoveImageWidth > INT_MIN && jhoveImageHeight > INT_MIN && jhoveFilesize > INT_MIN) {
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
            report.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(jhoveErrorMessage)));
        if (!jhoveErrorOutput.isEmpty())
            report.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(jhoveErrorOutput.replace(QStringLiteral("###"), QStringLiteral("\n")))));

        report.append(QStringLiteral("</jhove>\n"));
        report.append(QStringLiteral("</fileanalysis>"));
        emit analysisReport(objectName(), report);
    }
}
