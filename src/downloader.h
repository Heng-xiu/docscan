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

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QUrl>

#include "watchable.h"

/**
 * Common class for classes that download files based on URLs
 * or at least pretend to do so.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class Downloader : public QObject, public Watchable
{
    Q_OBJECT
public:
    explicit Downloader(QObject *parent = nullptr);

signals:
    void downloaded(QString);
    void downloaded(QUrl, QString);
    void report(QString, QString);

public slots:
    /**
     * Download file as specified by the url.
     *
     * @param url download from url
     */
    virtual void download(const QUrl &) = 0;

    /**
     * Request to log a summary of all download requests (e.g. success/failure rate).
     */
    virtual void finalReport() = 0;
};

#endif // DOWNLOADER_H
