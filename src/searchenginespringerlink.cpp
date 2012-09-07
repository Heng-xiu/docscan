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

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextStream>
#include <QUrl>
#include <QDebug>
#include <QCoreApplication>
#include <QRegExp>

#include "networkaccessmanager.h"
#include "general.h"
#include "searchenginespringerlink.h"

const QString SearchEngineSpringerLink::NoCategory = QLatin1String("content");
const int SearchEngineSpringerLink::NoYear = -1;

SearchEngineSpringerLink::SearchEngineSpringerLink(NetworkAccessManager *networkAccessManager, const QString &searchTerm, const QString &category, int year, QObject *parent)
    : SearchEngineAbstract(parent), m_networkAccessManager(networkAccessManager), m_searchTerm(searchTerm), m_category(category), m_year(year), m_isRunning(false)
{
    // TODO
}

void SearchEngineSpringerLink::startSearch(int numExpectedHits)
{
    m_numExpectedHits = numExpectedHits;
    m_numFoundHits = 0;
    m_searchOffset = 0;
    nextSearchStep();
}

bool SearchEngineSpringerLink::isAlive()
{
    return m_isRunning;
}

void SearchEngineSpringerLink::nextSearchStep()
{
    m_isRunning = true;

    QUrl url(QString("http://www.springerlink.com/%1/").arg(m_category));
    if (!m_searchTerm.isEmpty())
        url.addQueryItem("k", m_searchTerm);
    if (m_searchOffset > 0)
        url.addQueryItem("o", QString::number(m_searchOffset));
    if (m_year >= 1970)
        url.addQueryItem("Copyright", QString::number(m_year));

    DocScan::XMLNode reportNode;
    reportNode.name = QLatin1String("searchengine");
    reportNode.attributes.insert(QLatin1String("type"), QLatin1String("springerlink"));
    reportNode.attributes.insert(QLatin1String("search"), url.toString());
    emit report(DocScan::xmlNodeToText(reportNode));

    qDebug() << "m_networkAccessManager->get " << url.toString();
    QNetworkRequest request(url);
    m_networkAccessManager->setRequestHeaders(request);
    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void SearchEngineSpringerLink::finished()
{
    qDebug() << "SearchEngineSpringerLink::finished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
            QUrl newUrl = reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            qDebug() << "Redirection from" << reply->url().toString() << "to" << newUrl.toString();
            QNetworkRequest request(newUrl);
            m_networkAccessManager->setRequestHeaders(request);
            QNetworkReply *newReply = m_networkAccessManager->get(request);
            connect(newReply, SIGNAL(finished()), this, SLOT(finished()));
        } else {
            QTextStream tsAll(reply);
            QString htmlText = tsAll.readAll();

            if (m_searchOffset == 0) {
                /// first page

                static const QRegExp regExpNumHits(QLatin1String("Viewing items \\d+ - \\d+ of ([0-9,.]+)"));
                int p1 = htmlText.indexOf(regExpNumHits);
                if (p1 >= 0) {
                    DocScan::XMLNode numHitsNode;
                    numHitsNode.name = QLatin1String("searchengine");
                    numHitsNode.attributes.insert(QLatin1String("type"), QLatin1String("springerlink"));
                    numHitsNode.attributes.insert(QLatin1String("numresults"), regExpNumHits.cap(0));
                    emit report(DocScan::xmlNodeToText(numHitsNode));
                }
            }

            int p1 = -1;
            while ((p1 = htmlText.indexOf(QLatin1String("<li class=\"pdf\"><a"), p1 + 1)) >= 0) {
                int p2 = htmlText.indexOf(QLatin1String("href=\""), p1 + 18);
                if (p2 < 0) break;
                int p3 = htmlText.indexOf(QLatin1String("\" "), p2 + 6);
                if (p3 < 0) break;

                QUrl url(QLatin1String("http://www.springerlink.com/"));
                url.setPath(htmlText.mid(p2 + 6, p3 - p2 - 6));
                qDebug() << url.toString();

                if (url.isValid() && (m_numFoundHits++ < m_numExpectedHits)) {

                    DocScan::XMLNode fileFinderNode;
                    fileFinderNode.name = QLatin1String("filefinder");
                    fileFinderNode.attributes.insert(QLatin1String("event"), QLatin1String("hit"));
                    fileFinderNode.attributes.insert(QLatin1String("href"), url.toString());
                    emit report(DocScan::xmlNodeToText(fileFinderNode));

                    emit foundUrl(url);
                }
            }

            if (m_numExpectedHits > m_searchOffset + 10) {
                m_searchOffset += 10;
                nextSearchStep();
            } else
                m_isRunning = false;
        }
    } else {
        DocScan::XMLNode reportNode;
        reportNode.name = QLatin1String("searchengine");
        reportNode.attributes.insert(QLatin1String("type"), QLatin1String("springerlink"));
        reportNode.attributes.insert(QLatin1String("search"), reply->url().toString());
        reportNode.attributes.insert(QLatin1String("status"), QLatin1String("error"));
        emit report(DocScan::xmlNodeToText(reportNode));
        m_isRunning = false;
    }

    QCoreApplication::instance()->processEvents();
}
