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

#ifndef FILESYSTEMSCAN_H
#define FILESYSTEMSCAN_H

#include <QStringList>

#include "filefinder.h"

/**
 * Scan a file system tree starting from a base directory
 * and signal found files as if they were found URLs.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileSystemScan : public FileFinder
{
    Q_OBJECT
public:
    /**
     * Start searching from a base directory, but only report files matching a given filter.
     *
     * @param filters list of filters following for format specified in QDir
     * @param baseDir local directory to start searching from
     */
    explicit FileSystemScan(const QStringList &filters, const QString &baseDir, QObject *parent = nullptr);

    virtual void startSearch(int numExpectedHits);
    virtual bool isAlive();

private:
    const QStringList m_filters;
    const QString m_baseDir;
    bool m_alive;
};

#endif // FILESYSTEMSCAN_H
