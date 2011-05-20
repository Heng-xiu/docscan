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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegExp>
#include <QTextStream>
#include <QDebug>

#include "searchenginebing.h"
#include "general.h"

SearchEngineBing::SearchEngineBing(QNetworkAccessManager *networkAccessManager, const QString &searchTerm, QObject *parent)
    : SearchEngineAbstract(parent), m_networkAccessManager(networkAccessManager), m_searchTerm(searchTerm), m_numExpectedHits(0)
{
    // nothing
}

void SearchEngineBing::startSearch(int num)
{
    ++m_runningSearches;

    QUrl url(QLatin1String("http://www.bing.com/search?setmkt=en-US&setlang=match"));
    url.addQueryItem("q", m_searchTerm);
    m_numExpectedHits = num;
    m_currentPage = 0;
    m_numFoundHits = 0;

    emit report(QString("<searchengine type=\"bing\" search=\"%1\"/>\n").arg(DocScan::xmlify(url.toString())));

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

bool SearchEngineBing::isAlive()
{
    return m_runningSearches > 0;
}

void SearchEngineBing::finished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QTextStream tsAll(reply);
        QString htmlText = tsAll.readAll();

        if (m_currentPage == 0) {
            const QRegExp countHitsRegExp("of.{,10} ([0-9 ]+) result");
            if (countHitsRegExp.indexIn(htmlText) >= 0)
                emit report(QString("<searchengine type=\"bing\" numresults=\"%1\"/>\n").arg(countHitsRegExp.cap(1).replace(" ", "")));
        }

        const QRegExp searchHitRegExp("<h3><a href=\"([^\"]+)\"");
        int p = -1;
        while ((p = searchHitRegExp.indexIn(htmlText, p + 1)) >= 0) {
            QUrl url(searchHitRegExp.cap(1));
            if (url.isValid()) {
                emit foundUrl(url);
                ++m_numFoundHits;
                if (m_numFoundHits >= m_numExpectedHits) break;
            }
        }

        ++m_currentPage;
        if (m_currentPage * 10 < m_numExpectedHits) {
            QRegExp nextPageRegExp("<li><a href=\"(/search\\?q=[^\"]+)\"[^>]*>Next</a></li>");
            QUrl url(reply->url());
            if (nextPageRegExp.indexIn(htmlText) >= 0 && !nextPageRegExp.cap(1).isEmpty()) {
                url.setPath(nextPageRegExp.cap(1));
                emit report(QString("<searchengine type=\"bing\" search=\"%1\"/>\n").arg(DocScan::xmlify(url.toString())));
                ++m_runningSearches;
                reply = m_networkAccessManager->get(QNetworkRequest(url));
                connect(reply, SIGNAL(finished()), this, SLOT(finished()));
            } else
                emit result(ResultUnspecifiedError);
        } else
            emit result(ResultNoError);
    } else {
        QString logText = QString("<searchengine type=\"bing\" url=\"%1\" status=\"error\"/>\n").arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    }

    --m_runningSearches;
}
