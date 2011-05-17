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

#include <QTimer>
#include <QDebug>

#include "watchdog.h"
#include "watchable.h"

static const int countDownInit = 3;

WatchDog::WatchDog(QObject *parent)
    : QObject(parent), m_countDown(countDownInit)
{
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
    foreach(Watchable *watchable, m_watchables) {
        anyAlive |= watchable->isAlive();
        if (anyAlive) break;
    }

    if (anyAlive)
        m_countDown = countDownInit;
    else
        --m_countDown;

    if (m_countDown <= 0) {
        emit allDead();
        m_timer.stop();
    }
}
