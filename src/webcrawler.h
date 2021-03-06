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

#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <QList>
#include <QSet>
#include <QStringList>
#include <QSslError>

#include "filefinder.h"

class QTimer;
class QNetworkReply;
class QSignalMapper;
class QMutex;

class NetworkAccessManager;

/**
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class WebCrawler : public FileFinder
{
    Q_OBJECT
public:
    static const int maxVisitedPages;

    explicit WebCrawler(NetworkAccessManager *networkAccessManager, const QStringList &filters, const QUrl &baseUrl, const QUrl &startUrl, const QRegExp &requiredContent, int maxVisitedPages = WebCrawler::maxVisitedPages, QObject *parent = nullptr);
    ~WebCrawler();

    virtual void startSearch(int numExpectedHits);
    virtual bool isAlive();

private:
    static const QStringList blacklistHosts;

    typedef struct Filter {
        QString label;
        QRegExp regExp;
        int foundHits;
    } Filter;

    NetworkAccessManager *m_networkAccessManager;
    QString m_baseUrl, m_baseHost, m_startUrl;
    QRegExp m_requiredContent;
    QList<Filter> m_filterSet;
    bool m_terminating, m_shootingNextDownload;
    int m_runningDownloads;
    int m_numExpectedHits, m_visitedPages, m_maxVisitedPages;
    static const int maxParallelDownloads;
    int m_maxParallelDownloads;
    int m_interDownloadDelay;

    QSet<QNetworkReply *> *m_setRunningJobs;
    QMutex *m_mutexRunningJobs;
    QSignalMapper *m_signalMapperTimeout;

    QStringList m_knownUrls;
    QStringList m_queuedUrls;

    bool visitNextPage();
    QUrl normalizeUrl(const QString &partialUrl, const QUrl &baseUrl) const;
    bool isSubAddress(const QUrl &query, const QUrl &baseUrl);

private slots:
    void finishedDownload();
    void gotSslErrors(const QList<QSslError> &list);
    void timeout(QObject *object);
    void singleShotNextDownload();
};

#endif // WEBCRAWLER_H
