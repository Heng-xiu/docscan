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

#ifndef LOGCOLLECTOR_H
#define LOGCOLLECTOR_H

#include <QObject>
#include <QTextStream>
#include <QRegExp>
#include <QTextStream>

#include "watchable.h"

class QIODevice;

/**
 * Collecting log messages from various sources and
 * storing them in an IO device (e.g. file).
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class LogCollector : public QObject, public Watchable
{
    Q_OBJECT
public:
    /**
     * Create instance by specifying in which output device log messages
     * have to be stored.
     *
     * @param output device to log messages to
     */
    explicit LogCollector(QIODevice *output, QObject *parent = nullptr);

    virtual bool isAlive();

public slots:
    /**
     * Receive incomming log messages and store them in the output device
     * as specified in the constructor.
     *
     * @param message message to log
     */
    void receiveLog(const QString &origin, const QString &message);

    /**
     * Flush and close output device once logging is finished at process exit.
     */
    void close();

private:
    QTextStream m_ts;
    QIODevice *m_output;
    QRegExp m_tagStart;
};

#endif // LOGCOLLECTOR_H
