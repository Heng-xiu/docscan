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

SearchEngineBing::SearchEngineBing(const QString &searchTerm, QObject *parent)
    : SearchEngineAbstract(parent), m_searchTerm(searchTerm), m_numExpectedHits(0)
{
    m_nam = new QNetworkAccessManager(this);
}

SearchEngineBing::~SearchEngineBing()
{
    delete m_nam;
}

void SearchEngineBing::startSearch(int num)
{
    QUrl url(QLatin1String("http://www.bing.com/search?setmkt=en-US&setlang=match"));
    url.addQueryItem("q", m_searchTerm);
    m_numExpectedHits = num;
    m_currentPage = 0;

    connect(m_nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(receivedReply(QNetworkReply *)));
    m_nam->get(QNetworkRequest(url));
}

void SearchEngineBing::receivedReply(QNetworkReply *reply)
{
    const QRegExp searchHitRegExp("<h3><a href=\"([^\"]+)\"");
    QTextStream tsAll(reply);
    QString htmlText = tsAll.readAll();

    int p = -1;
    while ((p = searchHitRegExp.indexIn(htmlText, p + 1)) >= 0) {
        QUrl url(searchHitRegExp.cap(1));
        if (url.isValid())
            emit foundUrl(url);
    }

    ++m_currentPage;
    if (m_currentPage * 10 < m_numExpectedHits) {
        QRegExp nextPageRegExp("<li><a href=\"(/search\\?q=[^\"]+)\"[^>]*>Next</a></li>");
        QUrl url(reply->url());
        if (nextPageRegExp.indexIn(htmlText) >= 0 && !nextPageRegExp.cap(1).isEmpty()) {
            url.setPath(nextPageRegExp.cap(1));
            m_nam->get(QNetworkRequest(url));
        } else
            emit result(ResultUnspecifiedError);
    } else
        emit result(ResultNoError);
}
