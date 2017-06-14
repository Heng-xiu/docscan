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

#ifndef FROMLOGFILEFILEFINDER_H
#define FROMLOGFILEFILEFINDER_H

#include <QSet>
#include <QUrl>
#include <QPair>
#include <QStringList>

#include "filefinder.h"
#include "downloader.h"

/**
 * Extract URLs as reported in an older log file.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FromLogFileFileFinder : public FileFinder
{
    Q_OBJECT
public:
    /**
     * Create an instance and specify which old log file to search for urls ready to get downloaded.
     *
     * @param logfilename old log file to parse
     * @param filters list of filters, i.e. which filenames to extract. May contain strings like "*.doc" or "*.pdf". If empty, all downloaded files will get "found".
     */
    explicit FromLogFileFileFinder(const QString &logfilename, const QStringList &filters, QObject *parent = nullptr);

    virtual void startSearch(int numExpectedHits);
    virtual bool isAlive();

private:
    QSet<QUrl> m_urlSet;
    bool m_isAlive;
    const QRegExp filenameRegExp;
};

/**
 * Extract downloaded files as reported successfully downloaded in an older log file.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FromLogFileDownloader: public Downloader
{
    Q_OBJECT
public:
    /**
     * Create an instance and specify which old log file to search for successful downloads and which filter to apply.
     *
     * @param logfilename old log file to parse
     * @param filters list of filters, i.e. which filenames to extract. May contain strings like "*.doc" or "*.pdf". If empty, all downloaded files will get "found".
     */
    explicit FromLogFileDownloader(const QString &logfilename, const QStringList &filters, QObject *parent = nullptr);

    virtual bool isAlive();

public slots:
    void download(const QUrl &);
    void finalReport();

private slots:
    void startParsingAndEmitting();

private:
    QString m_logfilename;
    bool m_isAlive;
    const QRegExp filenameRegExp;
};

#endif // FROMLOGFILEFILEFINDER_H
