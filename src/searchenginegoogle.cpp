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

#include "searchenginegoogle.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegExp>
#include <QTextStream>
#include <QDebug>
#include <QUrlQuery>
#include <QCoreApplication>

#include "networkaccessmanager.h"
#include "general.h"

SearchEngineGoogle::SearchEngineGoogle(NetworkAccessManager *networkAccessManager, const QString &searchTerm, QObject *parent)
    : SearchEngineAbstract(parent), m_networkAccessManager(networkAccessManager), m_searchTerm(searchTerm), m_numExpectedHits(0), m_runningSearches(0)
{
    // nothing
}

void SearchEngineGoogle::startSearch(int num)
{
    ++m_runningSearches;
    m_hitsPerPage = qMin(num, defaultHitsPerPage);

    QUrl url(QString(QStringLiteral("http://www.google.com/search?hl=en&prmd=ivns&filter=0&ie=UTF-8&oe=UTF-8&num=%1")).arg(m_hitsPerPage));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"), m_searchTerm);
    url.setQuery(query);
    m_numExpectedHits = num;
    m_currentPage = 0;
    m_numFoundHits = 0;

    emit report(QString(QStringLiteral("<searchengine search=\"%1\" type=\"google\" />\n")).arg(DocScan::xmlify(url.toString())));

    QNetworkRequest request(url);
    m_networkAccessManager->setRequestHeaders(request);
    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

bool SearchEngineGoogle::isAlive()
{
    return m_runningSearches > 0;
}

void SearchEngineGoogle::finished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QString htmlText = QString::fromUtf8(reply->readAll().constData()).replace(QStringLiteral("&#160;"), QStringLiteral(" "));

        if (m_currentPage == 0) {
            /// Google has different layouts for web result pages, so different regular expressions are necessary
            const QRegExp countHitsRegExp1(QStringLiteral("of (about )?<b>([0-9][0-9,. ]*)</b> for"));
            if (countHitsRegExp1.indexIn(htmlText) >= 0)
                emit report(QString(QStringLiteral("<searchengine numresults=\"%1\" type=\"google\" />\n")).arg(countHitsRegExp1.cap(2).remove(QRegExp(QStringLiteral("[, .]+")))));
            else {
                const QRegExp countHitsRegExp2("\\b([0-9][0-9,. ]*) results");
                if (countHitsRegExp2.indexIn(htmlText) >= 0)
                    emit report(QString(QStringLiteral("<searchengine numresults=\"%1\" type=\"google\" />\n")).arg(countHitsRegExp2.cap(1).remove(QRegExp(QStringLiteral("[, .]+")))));
                else
                    emit report(QStringLiteral("<searchengine type=\"google\">\nCannot determine number of results\n</searchengine>\n"));
            }
        }

        const QRegExp searchHitRegExp(QStringLiteral("<h3 class=\"r\"><a href=\"([^\"]+)\""));
        int p = -1;
        while ((p = searchHitRegExp.indexIn(htmlText, p + 1)) >= 0) {
            QString urlText = searchHitRegExp.cap(1);

            /// Clean up Google's tracking URLs
            if (urlText.startsWith(QStringLiteral("/url?"))) {
                int p1 = urlText.indexOf(QStringLiteral("q="));
                int p2 = urlText.indexOf(QStringLiteral("&"), p1 + 1);
                if (p1 > 1) {
                    if (p2 < 0) p2 = urlText.length();
                    urlText = urlText.mid(p1 + 2, p2 - p1 - 2);
                }
            }

            QUrl url(urlText);
            if (url.isValid()) {
                ++m_numFoundHits;
                qDebug() << "Google found URL (" << m_numFoundHits << "of" << m_numExpectedHits << "):" << url.toString();
                emit report(QString(QStringLiteral("<filefinder event=\"hit\" href=\"%1\" />\n")).arg(DocScan::xmlify(url.toString())));
                emit foundUrl(url);
                if (m_numFoundHits >= m_numExpectedHits) break;
            }
        }

        ++m_currentPage;
        if (m_currentPage * m_hitsPerPage < m_numExpectedHits) {
            QUrl url(QString(QStringLiteral("http://www.google.com/search?hl=en&prmd=ivns&filter=0&ie=UTF-8&oe=UTF-8&num=%1")).arg(m_hitsPerPage));
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("q"), m_searchTerm);
            query.addQueryItem(QStringLiteral("start"), QString::number(m_currentPage * m_hitsPerPage));
            url.setQuery(query);
            emit report(QString(QStringLiteral("<searchengine search=\"%1\" type=\"google\" />\n")).arg(DocScan::xmlify(url.toString())));
            ++m_runningSearches;
            QNetworkRequest request(url);
            m_networkAccessManager->setRequestHeaders(request);
            reply = m_networkAccessManager->get(request);
            connect(reply, SIGNAL(finished()), this, SLOT(finished()));
        }
    } else {
        QString logText = QString(QStringLiteral("<searchengine status=\"error\" type=\"google\" url=\"%1\" />\n")).arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    }

    QCoreApplication::instance()->processEvents();

    --m_runningSearches;
}

const int SearchEngineGoogle::defaultHitsPerPage = 50;
