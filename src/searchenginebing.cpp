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

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegExp>
#include <QTextStream>
#include <QDebug>
#include <QUrlQuery>
#include <QCoreApplication>

#include "networkaccessmanager.h"
#include "searchenginebing.h"
#include "general.h"

SearchEngineBing::SearchEngineBing(NetworkAccessManager *networkAccessManager, const QString &searchTerm, QObject *parent)
    : SearchEngineAbstract(parent), m_networkAccessManager(networkAccessManager), m_searchTerm(searchTerm), m_numExpectedHits(0), m_runningSearches(0)
{
    // nothing
}

void SearchEngineBing::startSearch(int num)
{
    ++m_runningSearches;

    QUrl url(QStringLiteral("http://www.bing.com/search?setmkt=en-US&setlang=match"));
    QUrlQuery query;
    query.addQueryItem("q", m_searchTerm);
    url.setQuery(query);
    m_numExpectedHits = num;
    m_currentPage = 0;
    m_numFoundHits = 0;

    emit report(QString("<searchengine search=\"%1\" type=\"bing\" />\n").arg(DocScan::xmlify(url.toString())));

    QNetworkRequest request(url);
    m_networkAccessManager->setRequestHeaders(request);
    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

bool SearchEngineBing::isAlive()
{
    return m_runningSearches > 0;
}

void SearchEngineBing::finished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QString htmlText = QString::fromUtf8(reply->readAll().data()).replace(QStringLiteral("&#160;"), QStringLiteral(" "));

        if (m_currentPage == 0) {
            const QRegExp countHitsRegExp(QStringLiteral("([0-9]+([ ,][0-9]+)*) result"), Qt::CaseInsensitive);
            if (countHitsRegExp.indexIn(htmlText) >= 0) {
                const QString hits = countHitsRegExp.cap(1).replace(QRegExp(QStringLiteral("[, ]+")), QString());
                emit report(QString("<searchengine type=\"bing\" numresults=\"%1\" />\n").arg(hits));
            } else
                emit report(QStringLiteral("<searchengine type=\"bing\">\nCannot determine number of results\n</searchengine>\n"));
        }

        const QRegExp searchHitRegExp("<h3><a href=\"([^\"]+)\"");
        int p = -1;
        while ((p = searchHitRegExp.indexIn(htmlText, p + 1)) >= 0) {
            QUrl url(searchHitRegExp.cap(1));
            if (url.isValid()) {
                ++m_numFoundHits;
                qDebug() << "Bing found URL (" << m_numFoundHits << "of" << m_numExpectedHits << "): " << url.toString();
                emit report(QString("<filefinder event=\"hit\" href=\"%1\" />\n").arg(DocScan::xmlify(url.toString())));
                emit foundUrl(url);
                if (m_numFoundHits >= m_numExpectedHits) break;
            }
        }

        ++m_currentPage;
        if (m_currentPage * 10 < m_numExpectedHits) {
            QRegExp nextPageRegExp("<li><a href=\"(/search[^\"]+)\"[^>]*>Next</a></li>");
            QUrl url(reply->url());
            if (nextPageRegExp.indexIn(htmlText) >= 0 && !nextPageRegExp.cap(1).isEmpty()) {
                url.setPath(nextPageRegExp.cap(1));
                emit report(QString("<searchengine type=\"bing\" search=\"%1\" />\n").arg(DocScan::xmlify(url.toString())));
                ++m_runningSearches;
                QNetworkRequest request(url);
                m_networkAccessManager->setRequestHeaders(request);
                reply = m_networkAccessManager->get(request);
                connect(reply, SIGNAL(finished()), this, SLOT(finished()));
            } else
                qDebug() << "Cannot find link to continue";
        }
    } else {
        QString logText = QString("<searchengine type=\"bing\" url=\"%1\" status=\"error\" />\n").arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    }

    QCoreApplication::instance()->processEvents();

    --m_runningSearches;
}
