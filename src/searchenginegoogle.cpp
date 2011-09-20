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
#include <QCoreApplication>

#include "searchenginegoogle.h"
#include "general.h"

SearchEngineGoogle::SearchEngineGoogle(QNetworkAccessManager *networkAccessManager, const QString &searchTerm, QObject *parent)
    : SearchEngineAbstract(parent), m_networkAccessManager(networkAccessManager), m_searchTerm(searchTerm), m_numExpectedHits(0), m_runningSearches(0)
{
    // nothing
}

void SearchEngineGoogle::startSearch(int num)
{
    ++m_runningSearches;
    m_hitsPerPage = qMin(num, m_hitsPerPage);

    QUrl url(QString("http://www.google.com/search?hl=en&prmd=ivns&filter=0&ie=UTF-8&oe=UTF-8&num=%1").arg(m_hitsPerPage));
    url.addQueryItem("q", m_searchTerm);
    m_numExpectedHits = num;
    m_currentPage = 0;
    m_numFoundHits = 0;

    emit report(QString("<searchengine type=\"google\" search=\"%1\" />\n").arg(DocScan::xmlify(url.toString())));

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

bool SearchEngineGoogle::isAlive()
{
    return m_runningSearches > 0;
}

void SearchEngineGoogle::finished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QTextStream tsAll(reply);
        QString htmlText = tsAll.readAll();

        if (m_currentPage == 0) {
            const QRegExp countHitsRegExp("([0-9,. ]+) result[s]?");
            if (countHitsRegExp.indexIn(htmlText) >= 0)
                emit report(QString("<searchengine type=\"google\" numresults=\"%1\" />\n").arg(countHitsRegExp.cap(1).replace(QRegExp("[, .]"), "")));
        }

        const QRegExp searchHitRegExp("<h3 class=\"r\"><a href=\"([^\"]+)\"");
        int p = -1;
        while ((p = searchHitRegExp.indexIn(htmlText, p + 1)) >= 0) {
            QUrl url(searchHitRegExp.cap(1));
            if (url.isValid()) {
                ++m_numFoundHits;
                qDebug() << "Google found URL (" << m_numFoundHits << "of" << m_numExpectedHits << "):" << url.toString();
                emit report(QString("<filefinder event=\"hit\" href=\"%1\" />\n").arg(url.toString()));
                emit foundUrl(url);
                if (m_numFoundHits >= m_numExpectedHits) break;
            }
        }

        ++m_currentPage;
        if (m_currentPage * m_hitsPerPage < m_numExpectedHits) {
            QUrl url(QString("http://www.google.com/search?hl=en&prmd=ivns&filter=0&ie=UTF-8&oe=UTF-8&num=%1").arg(m_hitsPerPage));
            url.addQueryItem("q", m_searchTerm);
            url.addQueryItem("start", QString::number(m_currentPage * m_hitsPerPage));
            emit report(QString("<searchengine type=\"google\" search=\"%1\" />\n").arg(DocScan::xmlify(url.toString())));
            ++m_runningSearches;
            reply = m_networkAccessManager->get(QNetworkRequest(url));
            connect(reply, SIGNAL(finished()), this, SLOT(finished()));
        }
    } else {
        QString logText = QString("<searchengine type=\"google\" url=\"%1\" status=\"error\" />\n").arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    }

    QCoreApplication::instance()->processEvents();

    --m_runningSearches;
}

const int SearchEngineGoogle::defaulthitsPerPage = 50;
