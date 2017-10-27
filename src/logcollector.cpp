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

#include "logcollector.h"

#include <typeinfo>

#include <QDateTime>

#include "general.h"

LogCollector::LogCollector(QIODevice *output, QObject *parent)
    : QObject(parent), m_ts(output), m_output(output), m_tagStart(QStringLiteral("<(\\w+)\\b"))
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());
    m_ts << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl << "<log isodate=\"" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << "\">" << endl;

    logGitVersion();
}

bool LogCollector::isAlive()
{
    return false;
}

void LogCollector::receiveLog(const QString &origin, const QString &message)
{
    if (m_output->isOpen()) {
        const QString time = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        m_ts << "<logitem epoch=\"" << (QDateTime::currentMSecsSinceEpoch() / 1000) << "\" source=\"" << origin << "\" time=\"" << time << "\">" << endl << message << "</logitem>" << endl;
    }
}

void LogCollector::close()
{
    if (m_output->isOpen()) {
        m_ts << "</log>" << endl << "<!-- " << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << " -->" << endl;
        m_ts.flush();
    }
    m_output->close();
}

void LogCollector::logGitVersion() {
    static const QString gitCommit(QStringLiteral(GIT_COMMIT));
    static const QString gitCommitCount(QStringLiteral(GIT_COMMIT_COUNT));
    static const QDateTime gitCommitDate = QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1000) * GIT_COMMIT_DATE);
    if (!gitCommit.isEmpty() && !gitCommitCount.isEmpty()) {
        static const QString message(QString(QStringLiteral("<git commit=\"%1\" commitcount=\"%2\">\n%3</git>\n")).arg(gitCommit, gitCommitCount, DocScan::formatDateTime(gitCommitDate, QStringLiteral("commitdate"))));
        receiveLog(QStringLiteral("logcollector"), message);
    }
}
