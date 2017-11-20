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

#include "jhovewrapper.h"

#include <QProcess>
#include <QDebug>
#include <QRegularExpression>

#include "general.h"

static const int oneMinuteInMillisec = 60000;
static const int twoMinutesInMillisec = oneMinuteInMillisec * 2;
static const int fourMinutesInMillisec = oneMinuteInMillisec * 4;
static const int sixMinutesInMillisec = oneMinuteInMillisec * 6;

QString JHoveWrapper::setupJhove(QObject *parent, const QString &_jhoveShellscript)
{
    if (_jhoveShellscript.isEmpty() || parent == nullptr) {
        /// Invalid arguments, don't do anything
        return QString();
    } else if (jhoveShellscript == _jhoveShellscript) {
        /// Previously initialized, no need to do anything more
        return QString();
    }

    jhoveShellscript = _jhoveShellscript;

    QProcess jhove(parent);
    const QStringList arguments = QStringList() << QStringLiteral("--version");
    jhove.start(jhoveShellscript, arguments, QIODevice::ReadOnly);
    const bool jhoveStarted = jhove.waitForStarted(oneMinuteInMillisec);
    if (!jhoveStarted)
        qWarning() << "Failed to start jHove to retrieve version";
    else {
        const bool jhoveExited = jhove.waitForFinished(oneMinuteInMillisec);
        if (!jhoveExited)
            qWarning() << "Failed to finish jHove to retrieve version";
        else {
            const int jhoveExitCode = jhove.exitCode();
            const QString jhoveStandardOutput = QString::fromUtf8(jhove.readAllStandardOutput().constData()).trimmed();
            const QString jhoveErrorOutput = QString::fromUtf8(jhove.readAllStandardError().constData()).trimmed();
            static const QRegularExpression regExpVersionNumber(QStringLiteral("Jhove \\(Rel\\. (([0-9]+[.])+[0-9]+)"));
            const QRegularExpressionMatch regExpVersionNumberMatch = regExpVersionNumber.match(jhoveStandardOutput);
            const bool status = jhoveExitCode == 0 && regExpVersionNumberMatch.hasMatch();
            const QString versionNumber = status ? regExpVersionNumberMatch.captured(1) : QString();

            QString report = QString(QStringLiteral("<toolcheck name=\"jhove\" exitcode=\"%1\" status=\"%2\"%3>\n")).arg(jhoveExitCode).arg(status ? QStringLiteral("ok") : QStringLiteral("error")).arg(status ? QString(QStringLiteral(" version=\"%1\"")).arg(versionNumber) : QString());
            if (!jhoveStandardOutput.isEmpty())
                report.append(QStringLiteral("<output>")).append(DocScan::xmlifyLines(jhoveStandardOutput)).append(QStringLiteral("</output>\n"));
            if (!jhoveErrorOutput.isEmpty())
                report.append(QStringLiteral("<error>")).append(DocScan::xmlifyLines(jhoveErrorOutput)).append(QStringLiteral("</error>\n"));
            report.append(QStringLiteral("</toolcheck>\n"));
            return report;
        }
    }

    return QString();
}

QProcess *JHoveWrapper::launchJHove(QObject *parent, const Module module, const QString &filename) {
    const QString moduleName = hulName(module);
    if (!jhoveShellscript.isEmpty() && !moduleName.isEmpty()) {
        /// External programs should be both CPU and I/O 'nice'
        static const QStringList defaultArgumentsForNice = QStringList() << QStringLiteral("-n") << QStringLiteral("17") << QStringLiteral("ionice") << QStringLiteral("-c") << QStringLiteral("3");

        QProcess *jhoveProcess = new QProcess(parent);
        const QString quotedFilename = filename.contains(QLatin1Char(' ')) ? QLatin1Char('"') + filename + QLatin1Char('"') : filename;
        const QStringList arguments = QStringList(defaultArgumentsForNice) << QStringLiteral("/bin/bash") << jhoveShellscript << QStringLiteral("-m") << moduleName << QStringLiteral("-t") << QStringLiteral("/tmp") << QStringLiteral("-b") << QStringLiteral("131072") << quotedFilename;
        jhoveProcess->start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        return jhoveProcess;
    } else
        return nullptr;
}

QString JHoveWrapper::hulName(Module module) {
    switch (module) {
    case JHovePDF: return QStringLiteral("PDF-hul");
    case JHoveJPEG: return QStringLiteral("JPEG-hul");
    case JHoveJPEG2000: return QStringLiteral("JPEG2000-hul");
    case JHoveTIFF: return QStringLiteral("TIFF-hul");
    default:
        qWarning() << "Unchecked case for module=" << module;
        return QString();
    }
}

QString JHoveWrapper::jhoveShellscript;
