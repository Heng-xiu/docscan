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

#include <QTextStream>
#include <QDateTime>

#include "logcollector.h"

LogCollector::LogCollector(QIODevice &output, QObject *parent)
    : QObject(parent), m_ts(&output), m_tagStart("<(\\w+)\\b")
{
}

bool LogCollector::isAlive()
{
    return false;
}

void LogCollector::receiveLog(QString message)
{
    int i = m_tagStart.indexIn(message);
    if (i >= 0) {
        int j = i + m_tagStart.cap(0).length();
        QString time = QDateTime::currentDateTime().toUTC().toString(Qt::ISODate);
        message = message.left(j) + " time=\"" + time + "\"" + message.mid(j);
    }
    m_ts << message;
    m_ts.flush();
}
