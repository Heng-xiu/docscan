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
#include <QNetworkReply>
#include <QTimer>
#include <QRegExp>
#include <QSignalMapper>
#include <QMutex>

#include "webcrawler.h"
#include "general.h"

WebCrawler::WebCrawler(QNetworkAccessManager *networkAccessManager, const QStringList &filters, const QUrl &baseUrl, QObject *parent)
    : FileFinder(parent), m_networkAccessManager(networkAccessManager), m_baseUrl(baseUrl), m_runningDownloads(0)
{
    m_signalMapperTimeout = new QSignalMapper(this);
    connect(m_signalMapperTimeout, SIGNAL(mapped(QObject *)), this, SLOT(timeout(QObject *)));
    m_setRunningJobs = new QSet<QNetworkReply *>();
    m_mutexRunningJobs = new QMutex();

    QString reText;
    foreach(QString filter, filters) {
        if (!reText.isEmpty()) reText += '|';
        reText += "(^|/)" + filter.replace(".", "\\.").replace("?", ".").replace("*", "[^ \"]*") + '$';
    }
    m_filePattern = QRegExp(reText);
}

WebCrawler::~WebCrawler() {
    delete m_signalMapperTimeout;
    delete m_setRunningJobs;
    delete m_mutexRunningJobs;
}

void WebCrawler::startSearch(int numExpectedHits)
{
    m_numExpectedHits = numExpectedHits;
    m_numFoundHits = 0;
    m_visitedPages = 0;

    m_queuedUrls.clear();
    m_knownUrls.clear();
    m_knownUrls << m_baseUrl.toString();

    startDownload(m_baseUrl);
}

bool WebCrawler::isAlive()
{
    return m_runningDownloads > 0;
}

void WebCrawler::startDownload(const QUrl &url)
{
    ++m_visitedPages;
    if (m_visitedPages > maxVisitedPages) return;

    ++m_runningDownloads;

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(finishedDownload()));

    m_mutexRunningJobs->lock();
    m_setRunningJobs->insert(reply);
    m_mutexRunningJobs->unlock();
    QTimer *timer = new QTimer(reply);
    connect(timer, SIGNAL(timeout()), m_signalMapperTimeout, SLOT(map()));
    m_signalMapperTimeout->setMapping(timer, reply);
    timer->start(15000 + m_runningDownloads * 200);

}

void WebCrawler::finishedDownload()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    m_mutexRunningJobs->lock();
    m_setRunningJobs->remove(reply);
    m_mutexRunningJobs->unlock();

    if (reply->error() == QNetworkReply::NoError) {
        const QString baseUrl = m_baseUrl.toString();
        QByteArray data(reply->readAll());
        QString text(data);

        /// check if HTML page ...
        if (text.left(256).toLower().contains("<html>")) {

            /// for each reference in the HTML code
            QRegExp referenceRegExp("<a\\b[^>]*href=\"([^\" \t><]+)\"");
            int p = -1;
            while ((p = text.indexOf(referenceRegExp, p + 1)) >= 0) {
                QString url = completeUrl(referenceRegExp.cap(1), reply->url()); // TODO complete
                if (url.isNull()) continue;

                if (m_numFoundHits < m_numExpectedHits && m_filePattern.indexIn(url) >= 0) {
                    ++m_numFoundHits;
                    emit report(QString("<filefinder event=\"hit\" href=\"%1\"/>\n").arg(url));
                    emit foundUrl(QUrl(url));
                } else if (url.startsWith(baseUrl) && (url.endsWith("/") || url.endsWith(".htm") || url.endsWith(".html")) && !m_knownUrls.contains(url)) {
                    m_knownUrls << url;
                    m_queuedUrls << url;
                }
            }

            emit report(QString("<webcrawler url=\"%1\" status=\"success\"/>\n").arg(DocScan::xmlify(reply->url().toString())));
        } else
            emit report(QString("<webcrawler url=\"%1\" detailed=\"Not an HTML page\" status=\"error\"/>\n").arg(DocScan::xmlify(reply->url().toString())));
    } else
        emit report(QString("<webcrawler url=\"%1\" detailed=\"%2\" status=\"error\"/>\n").arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(reply->errorString())));

    while (!m_queuedUrls.isEmpty() && m_runningDownloads < maxParallelDownloads && m_numFoundHits < m_numExpectedHits) {
        QString url = m_queuedUrls.first();
        m_queuedUrls.removeFirst();
        startDownload(QUrl(url));
    }

    --m_runningDownloads;
}

void WebCrawler::timeout(QObject *object)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(object);
    m_mutexRunningJobs->lock();
    if (m_setRunningJobs->contains(reply)) {
        m_setRunningJobs->remove(reply);
        m_mutexRunningJobs->unlock();
        reply->close();
        QString logText = QString("<download url=\"%1\" message=\"timeout\" status=\"error\"/>\n").arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    } else
        m_mutexRunningJobs->unlock();
}

QString WebCrawler::completeUrl(const QString &partialUrl, const QUrl &baseUrl)
{
    static QRegExp invalidProtocol("^[^:]{2,10}:");
    static QRegExp encodedCharHex("%([0-9a-fA-F]{2})");
    QString result = partialUrl;
    result = result.replace(QRegExp("#.*$"), "");

    while (encodedCharHex.indexIn(result) >= 0) {
        bool ok = false;
        QChar c(encodedCharHex.cap(1).toInt(&ok, 16));
        if (ok) {
            result = result.replace(encodedCharHex.cap(0), c);
        }
    }

    if (result.startsWith("http://") || result.startsWith("https://"))
        return result;
    else if (result.contains("://") || result.startsWith("#") || invalidProtocol.indexIn(result) >= 0)
        return QString::null;
    else if (result.startsWith("/"))
        return "http://" + baseUrl.host() + result;
    else {
        QString dir = baseUrl.toString().replace(QRegExp("[^/]+$"), "");
        return dir + result;
    }
}

const int WebCrawler::maxParallelDownloads = 4;
const int WebCrawler::maxVisitedPages = 512;
