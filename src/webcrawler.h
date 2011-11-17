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

#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <QList>
#include <QSet>
#include <QStringList>

#include "filefinder.h"

class QNetworkAccessManager;
class QTimer;
class QNetworkReply;
class QSignalMapper;
class QMutex;

/**
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class WebCrawler : public FileFinder
{
    Q_OBJECT
public:
    static const int maxVisitedPages;

    explicit WebCrawler(QNetworkAccessManager *networkAccessManager, const QStringList &filters, const QUrl &baseUrl, const QUrl &startUrl, const QRegExp &requiredContent, int maxVisitedPages = WebCrawler::maxVisitedPages, QObject *parent = 0);
    ~WebCrawler();

    virtual void startSearch(int numExpectedHits);
    virtual bool isAlive();

private:
    typedef struct Filter {
        QString label;
        QRegExp regExp;
        int foundHits;
    } Filter;

    QNetworkAccessManager *m_networkAccessManager;
    QString m_baseUrl, m_startUrl;
    QRegExp m_requiredContent;
    QList<Filter> m_filterSet;
    int m_runningDownloads;
    int m_numExpectedHits, m_visitedPages, m_maxVisitedPages;
    static const int maxParallelDownloads;

    QSet<QNetworkReply *> *m_setRunningJobs;
    QMutex *m_mutexRunningJobs;
    QSignalMapper *m_signalMapperTimeout;

    QStringList m_knownUrls;
    QStringList m_queuedUrls;

    bool startNextDownload();
    QString normalizeUrl(const QString &partialUrl, const QUrl &baseUrl);
    bool isSubAddress(const QUrl &query, const QUrl &baseUrl);

private slots:
    void finishedDownload();
    void timeout(QObject *object);
    void singleShotNextDownload();
};

#endif // WEBCRAWLER_H
