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

#ifndef WATCHABLE_H
#define WATCHABLE_H

/**
 * An interface to monitor if an object is still active,
 * e.g. waiting for another thread or network activity to finish.
 * Watchable objects are usually monitored by a WatchDog instance.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 * @see WatchDog
 */
class Watchable
{
public:
    /**
     * Test if an object is still active.
     * Has to be implemented by every class to be instanciated
     * and inheriting from this class.
     * @return 'true' if the object is still active, otherwise 'false'
     */
    virtual bool isAlive() = 0;
};

#endif // WATCHABLE_H
