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

#ifndef FROMLOGFILEFILEFINDER_H
#define FROMLOGFILEFILEFINDER_H

#include <QSet>
#include <QUrl>
#include <QPair>
#include <QStringList>

#include "filefinder.h"
#include "downloader.h"

class FromLogFileFileFinder : public FileFinder
{
    Q_OBJECT
public:
    explicit FromLogFileFileFinder(const QString &logfilename, const QStringList &filters, QObject *parent = 0);

    virtual void startSearch(int numExpectedHits);
    virtual bool isAlive();

signals:
    void foundUrl(QUrl);

private:
    QSet<QUrl> m_urlSet;
    bool m_isAlive;
};

class FromLogFileDownloader: public Downloader
{
    Q_OBJECT
public:
    explicit FromLogFileDownloader(const QString &logfilename, const QStringList &filters, QObject *parent = 0);

    virtual bool isAlive();

signals:
    void downloaded(QUrl, QString);
    void downloaded(QString);
    void report(QString);

public slots:
    void download(const QUrl &);
    void finalReport();

private slots:
    void startParsingAndEmitting();

private:
    QString m_logfilename;
    bool m_isAlive;
    const QStringList &m_filters;
};

#endif // FROMLOGFILEFILEFINDER_H
