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

#include "searchenginespringerlink.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextStream>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QCoreApplication>
#include <QRegExp>

#include "networkaccessmanager.h"
#include "general.h"

const QString SearchEngineSpringerLink::AllCategories = QStringLiteral("content");
const int SearchEngineSpringerLink::AllYears = -1;
const QString SearchEngineSpringerLink::AllSubjects;
const QString SearchEngineSpringerLink::AllContentTypes;

SearchEngineSpringerLink::SearchEngineSpringerLink(NetworkAccessManager *networkAccessManager, const QString &searchTerm, const QString &category, const QString &contentType, const QString &subject, int year, QObject *parent)
    : SearchEngineAbstract(parent), m_networkAccessManager(networkAccessManager), m_searchTerm(searchTerm), m_category(category), m_contentType(contentType), m_subject(subject), m_year(year), m_isRunning(false)
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());
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

    QUrl url(QString(QStringLiteral("http://www.springerlink.com/%1/")).arg(m_category));
    QUrlQuery query;
    if (!m_searchTerm.isEmpty())
        query.addQueryItem(QStringLiteral("k"), m_searchTerm);
    if (!m_contentType.isEmpty())
        query.addQueryItem(QStringLiteral("Content+Type"), m_contentType);
    if (!m_subject.isEmpty())
        query.addQueryItem(QStringLiteral("Subject"), m_subject);
    if (m_searchOffset > 0)
        query.addQueryItem(QStringLiteral("o"), QString::number(m_searchOffset));
    if (m_year >= 1970)
        query.addQueryItem(QStringLiteral("Copyright"), QString::number(m_year));
    query.addQueryItem(QStringLiteral("Language"), QStringLiteral("English"));
    url.setQuery(query);

    DocScan::XMLNode reportNode;
    reportNode.name = QStringLiteral("searchengine");
    reportNode.attributes.insert(QStringLiteral("type"), QStringLiteral("springerlink"));
    reportNode.attributes.insert(QStringLiteral("search"), url.toString());
    emit report(objectName(), DocScan::xmlNodeToText(reportNode));

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
            QString htmlText = QString::fromUtf8(reply->readAll().constData()).replace(QStringLiteral("&#160;"), QStringLiteral(" "));

            if (m_searchOffset == 0) {
                /// first page

                static const QRegExp regExpNumHits(QStringLiteral("Viewing items \\d+ - \\d+ of ([0-9,.]+)"));
                int p1 = regExpNumHits.indexIn(htmlText);
                if (p1 >= 0) {
                    QString numHits = regExpNumHits.cap(1).remove(QStringLiteral(","));
                    DocScan::XMLNode numHitsNode;
                    numHitsNode.name = QStringLiteral("searchengine");
                    numHitsNode.attributes.insert(QStringLiteral("type"), QStringLiteral("springerlink"));
                    numHitsNode.attributes.insert(QStringLiteral("numresults"), numHits);
                    emit report(objectName(), DocScan::xmlNodeToText(numHitsNode));
                }
            }

            int p1 = -1;
            while ((p1 = htmlText.indexOf(QStringLiteral("<li class=\"pdf\"><a"), p1 + 1)) >= 0) {
                int p2 = htmlText.indexOf(QStringLiteral("href=\""), p1 + 18);
                if (p2 < 0) break;
                int p3 = htmlText.indexOf(QStringLiteral("\" "), p2 + 6);
                if (p3 < 0) break;

                QUrl url(QStringLiteral("http://www.springerlink.com/"));
                url.setPath(htmlText.mid(p2 + 6, p3 - p2 - 6));
                qDebug() << url.toString();

                if (url.isValid() && (m_numFoundHits++ < m_numExpectedHits)) {

                    DocScan::XMLNode fileFinderNode;
                    fileFinderNode.name = QStringLiteral("filefinder");
                    fileFinderNode.attributes.insert(QStringLiteral("event"), QStringLiteral("hit"));
                    fileFinderNode.attributes.insert(QStringLiteral("href"), url.toString());
                    emit report(objectName(), DocScan::xmlNodeToText(fileFinderNode));

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
        reportNode.name = QStringLiteral("searchengine");
        reportNode.attributes.insert(QStringLiteral("type"), QStringLiteral("springerlink"));
        reportNode.attributes.insert(QStringLiteral("search"), reply->url().toString());
        reportNode.attributes.insert(QStringLiteral("status"), QStringLiteral("error"));
        emit report(objectName(), DocScan::xmlNodeToText(reportNode));
        m_isRunning = false;
    }

    QCoreApplication::instance()->processEvents();
}
