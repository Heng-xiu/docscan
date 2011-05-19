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

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <QObject>
#include <QTimer>

class Watchable;

class WatchDog : public QObject
{
    Q_OBJECT
public:
    explicit WatchDog(QObject *parent = 0);

    void addWatchable(Watchable *watchable);

signals:
    void aboutToQuit();
    void quit();

private:
    QTimer m_timer;
    QList<Watchable *> m_watchables;
    int m_countDown;

private slots:
    void watch();
};

#endif // WATCHDOG_H
