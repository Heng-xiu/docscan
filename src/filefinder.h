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

class FileFinder : public QObject, public Watchable
{
    Q_OBJECT
public:
    static const int ResultNoError;
    static const int ResultUnspecifiedError;

    explicit FileFinder(QObject *parent = 0);

    virtual void startSearch(int numExpectedHits) = 0;

signals:
    void foundUrl(QUrl);
    void result(int);
    void report(QString);
};

#endif // FILEFINDER_H
