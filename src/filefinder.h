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

#ifndef FILEFINDER_H
#define FILEFINDER_H

#include <QObject>
#include <QUrl>

#include "watchable.h"

/**
 * Common class used to organize the search for files. Possible implementations
 * include both online searches via search engines or web crawlers as well as
 * searches on local storages such as recursively inside directories.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileFinder : public QObject, public Watchable
{
    Q_OBJECT
public:
    /// Result value for various functions if function had no error.
    static const int ResultNoError;
    /// Result value for various functions if function had some error (not specified).
    static const int ResultUnspecifiedError;

    explicit FileFinder(QObject *parent = nullptr);

    /**
     * Start searching as specified in another function.
     * Although the number of expected hits can be specified,
     * there is no guarantee that that many hits get actually found.
     *
     * @param numExpectedHits number of expected hits.
     */
    virtual void startSearch(int numExpectedHits) = 0;

signals:
    /**
     * Notification about a found url that can be downloaded
     * or otherwise processed.
     */
    void foundUrl(QUrl);

    /**
     * Log message about an event that should be reported.
     */
    void report(QString);
};

#endif // FILEFINDER_H
