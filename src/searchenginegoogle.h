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

#ifndef SEARCHENGINEGOOGLE_H
#define SEARCHENGINEGOOGLE_H

#include <QObject>

#include "searchengineabstract.h"

class QNetworkReply;

class NetworkAccessManager;

/**
 * Search engine using Google to search for files.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class SearchEngineGoogle : public SearchEngineAbstract
{
    Q_OBJECT
public:
    explicit SearchEngineGoogle(NetworkAccessManager *networkAccessManager, const QString &searchTerm, QObject *parent = 0);

    virtual void startSearch(int numExpectedHits);
    virtual bool isAlive();

private:
    NetworkAccessManager *m_networkAccessManager;
    const QString m_searchTerm;
    int m_numExpectedHits, m_currentPage, m_numFoundHits;
    int m_runningSearches;
    int m_hitsPerPage;
    static const int defaultHitsPerPage;

private slots:
    void finished();
};

#endif // SEARCHENGINEGOOGLE_H
