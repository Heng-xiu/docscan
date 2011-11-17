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
#include <QSet>

class Watchable;

/**
 * Monitor to watch if the objects under surveillance are still active.
 * Initially, objects inheriting Watchable have to be added to the set
 * of monitored objects. The watch dog object will test the objects in
 * regular intervals if they alive. If no object is alive, a sequence of
 * signals will be issued.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class WatchDog : public QObject
{
    Q_OBJECT
public:
    explicit WatchDog(QObject *parent = 0);

    /**
     * Add an object to the set of objects to be watched.
     * Each added object will be regularly tested if it is alive.
     * @param watchable object to be watched
     * @see Watchable
     */
    void addWatchable(Watchable *watchable);

signals:
    /**
     * First warning issued if all objects are no longer alive
     * for a short time. If any object switches back to be alive
     * again, the grace period is resetted.
     */
    void firstWarning();

    /**
     * Last warning issued if all objects are no longer alive
     * for some time. If any object switches back to be alive
     * again, the grace period is resetted.
     */
    void lastWarning();

    /**
     * Special signal for use to connec to QApplication::quit
     * if all objects are no longer alive for a longer time.
     */
    void quit();

private:
    QTimer m_timer;
    QSet<Watchable *> m_watchables;
    int m_countDown;

private slots:
    void watch();
};

#endif // WATCHDOG_H
