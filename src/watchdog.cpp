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

#include "watchdog.h"

#include <QTimer>
#include <QDebug>

#include "watchable.h"

static const int countDownInit = 6;

WatchDog::WatchDog(QObject *parent)
    : QObject(parent), m_countDown(countDownInit)
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(watch()));
    m_timer.setInterval(1000);
    m_timer.start();
}

void WatchDog::addWatchable(Watchable *watchable)
{
    m_watchables << watchable;
}

void WatchDog::watch()
{
    bool anyAlive = false;
    for (Watchable *watchable : const_cast<const QSet<Watchable *> &>(m_watchables)) {
        anyAlive |= watchable->isAlive();
        if (anyAlive) break;
    }

    if (anyAlive)
        m_countDown = countDownInit;
    else
        --m_countDown;

    if (m_countDown == countDownInit * 2 / 3) {
        qDebug() << "Watchdog gives first warning";
        emit firstWarning();
    } else if (m_countDown == countDownInit / 3) {
        qDebug() << "Watchdog gives last warning";
        emit lastWarning();
    } else  if (m_countDown == 0) {
        qDebug() << "Watchdog says quit now";
        emit quit();
        m_timer.stop();
    }
}
