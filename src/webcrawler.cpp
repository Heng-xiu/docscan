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

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QRegExp>
#include <QSignalMapper>
#include <QMutex>
#include <QDebug>

#include "webcrawler.h"
#include "general.h"

WebCrawler::WebCrawler(QNetworkAccessManager *networkAccessManager, const QStringList &filters, const QUrl &baseUrl, QObject *parent)
    : FileFinder(parent), m_networkAccessManager(networkAccessManager), m_baseUrl(QUrl(baseUrl).toString()), m_runningDownloads(0)
{
    m_signalMapperTimeout = new QSignalMapper(this);
    connect(m_signalMapperTimeout, SIGNAL(mapped(QObject *)), this, SLOT(timeout(QObject *)));
    m_setRunningJobs = new QSet<QNetworkReply *>();
    m_mutexRunningJobs = new QMutex();

    QString reText = QLatin1String("(^|/)(");
    bool first = true;
    foreach(QString filter, filters) {
        if (!first) reText += QChar('|'); first = false;
        reText += filter.replace(".", "\\.").replace("?", ".").replace("*", "[^ \"]*");
    }
    reText += QLatin1String(")$");
    m_filePattern = QRegExp(reText);
}

WebCrawler::~WebCrawler()
{
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
    m_queuedUrls << m_baseUrl;
    m_knownUrls.clear();
    m_knownUrls << m_baseUrl;

    emit report(QString("<webcrawler><filepattern>%1</filepattern></webcrawler>\n").arg(DocScan::xmlify(m_filePattern.pattern())));

    startNextDownload();
}

bool WebCrawler::isAlive()
{
    return m_runningDownloads > 0;
}

bool WebCrawler::startNextDownload()
{
    if (m_runningDownloads >= maxParallelDownloads || m_queuedUrls.isEmpty() || m_numFoundHits >= m_numExpectedHits)
        return false;

    ++m_visitedPages;
    if (m_visitedPages > maxVisitedPages)
        return false;

    ++m_runningDownloads;
    const QString url = m_queuedUrls.first();
    m_queuedUrls.removeFirst();
    qDebug() << "Starting download on " << url << "(" << m_visitedPages << ")";

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(QUrl(url)));
    connect(reply, SIGNAL(finished()), this, SLOT(finishedDownload()));

    m_mutexRunningJobs->lock();
    m_setRunningJobs->insert(reply);
    m_mutexRunningJobs->unlock();
    QTimer *timer = new QTimer(reply);
    connect(timer, SIGNAL(timeout()), m_signalMapperTimeout, SLOT(map()));
    m_signalMapperTimeout->setMapping(timer, reply);
    timer->start(10000 + m_runningDownloads * 1000);

    if (m_runningDownloads < maxParallelDownloads)
        startNextDownload();

    return true;
}

void WebCrawler::finishedDownload()
{
    QRegExp fileExtRegExp = QRegExp(QLatin1String("[.].{1,4}$"), Qt::CaseInsensitive);
    QRegExp validFileExtRegExp = QRegExp(QLatin1String("([.](htm[l]?|jsp|asp[x]?|php)|[^.]{5,})([?].+)?$"), Qt::CaseInsensitive);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    m_mutexRunningJobs->lock();
    m_setRunningJobs->remove(reply);
    m_mutexRunningJobs->unlock();

    /// check for redirections
    QString redirUrlStr = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
    if (!redirUrlStr.isEmpty()) {
        QUrl redirUrl = reply->url().resolved(QUrl(redirUrlStr));
        if (redirUrl.isValid() && !m_knownUrls.contains((redirUrlStr = redirUrl.toString()))) {
            m_knownUrls << redirUrlStr;
            m_queuedUrls << redirUrlStr;
        }
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data(reply->readAll());
        QString text(data);

        /// check if HTML page ...
        if (text.left(256).toLower().contains("<html")) {
            emit report(QString(QLatin1String("<webcrawler url=\"%1\" status=\"success\" />\n")).arg(DocScan::xmlify(reply->url().toString())));

            /// collect hits
            QSet<QString> hitCollection;

            /// for each anchor in the HTML code
            QRegExp anchorRegExp("<a\\b[^>]*href=\"([^\" \t><]+)\"");
            int p = -1;
            while ((p = text.indexOf(anchorRegExp, p + 1)) >= 0) {
                QString url = normalizeUrl(anchorRegExp.cap(1), reply->url());

                if (url.isEmpty()) continue;
                if (m_knownUrls.contains(url)) continue;

                /// simplification: extension (with or without dot) is four chars long
                QString extension = url.right(4).toLower();
                /// exclude images
                if (extension == QLatin1String(".jpg") || extension == QLatin1String("jpeg") || extension == QLatin1String(".png") || extension == QLatin1String(".gif"))
                    continue;
                /// exclude multimedia files
                if (extension == QLatin1String(".avi") || extension == QLatin1String("mpeg") || extension == QLatin1String(".mpg") || extension == QLatin1String(".mp4") || extension == QLatin1String(".mp3"))
                    continue;

                m_knownUrls << url;
                if (m_numFoundHits >= m_numExpectedHits) continue;

                if (m_filePattern.indexIn(url) >= 0) {
                    /// link matches requested file type
                    ++m_numFoundHits;
                    hitCollection.insert(url);
                } else if (!isSubAddress(QUrl(url), QUrl(m_baseUrl))) {
                    // qDebug() << "Is not a sub-address:" << url << "of" << m_baseUrl;
                } else if (!url.endsWith("/") && validFileExtRegExp.indexIn(url) == 0) {
                    // qDebug() << "Path or extension is not wanted" << url;
                } else if (extension == QLatin1String(".doc") || extension == QLatin1String("docx") || extension == QLatin1String(".rtf") || extension == QLatin1String(".pdf") || extension == QLatin1String(".odt")) {
                    // qDebug() << "Filename is an office document where is not considered for now" << url;
                } else
                    m_queuedUrls << url;
            }

            /// delay sending signals to ensure BFS on links
            foreach(const QString &url, hitCollection) {
                emit report(QString(QLatin1String("<filefinder event=\"hit\" href=\"%1\" />\n")).arg(DocScan::xmlify(url)));
                emit foundUrl(QUrl(url));
            }
        } else
            emit report(QString(QLatin1String("<webcrawler url=\"%1\" detailed=\"Not an HTML page\" status=\"error\" />\n")).arg(DocScan::xmlify(reply->url().toString())));
    } else
        emit report(QString(QLatin1String("<webcrawler url=\"%1\" detailed=\"%2\" status=\"error\" />\n")).arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(reply->errorString())));

    qApp->processEvents();
    --m_runningDownloads;

    bool downloadStarted = startNextDownload();

    if (!downloadStarted && m_runningDownloads == 0) {
        /// web crawler has started as there are no downloads active
        emit report(QString(QLatin1String("<webcrawler numfoundhits=\"%1\" numexpectedhits=\"%2\" numknownurls=\"%3\" numvisitedpages=\"%4\" />\n")).arg(m_numFoundHits).arg(m_numExpectedHits).arg(m_knownUrls.count()).arg(m_visitedPages));
    }
}

void WebCrawler::timeout(QObject *object)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(object);
    m_mutexRunningJobs->lock();
    if (m_setRunningJobs->contains(reply)) {
        m_setRunningJobs->remove(reply);
        m_mutexRunningJobs->unlock();
        reply->close();
        QString logText = QString("<download url=\"%1\" message=\"timeout\" status=\"error\" />\n").arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    } else
        m_mutexRunningJobs->unlock();
}

QString WebCrawler::normalizeUrl(const QString &partialUrl, const QUrl &baseUrl)
{
    static QRegExp protocol("^([^:]{2,10}):");
    static QRegExp encodedCharHex("%([0-9a-fA-F]{2})");

    /// complete URL to absolute URL
    QUrl urlObj = baseUrl.resolved(QUrl(partialUrl));
    if (urlObj.path().isEmpty()) urlObj.setPath(QChar('/'));
    QString result = urlObj.toString();

    if (protocol.indexIn(result) >= 0 && !protocol.cap(1).startsWith(QLatin1String("http")))
        return QString::null;

    /// remove JavaScript links or links pointed in page-internal anchors
    result = result.replace(QRegExp("#.*$"), "");

    /// resolve mime-encoded parts of the URL
    int p = -1;
    while ((p = encodedCharHex.indexIn(result, p + 1)) >= 0) {
        bool ok = false;
        QChar c(encodedCharHex.cap(1).toInt(&ok, 16));
        if (ok) {
            result = result.replace(encodedCharHex.cap(0), c);
        }
    }
    result = result.replace(QLatin1String("&amp;"), QChar('&'));

    return result;
}

bool WebCrawler::isSubAddress(const QUrl &query, const QUrl &baseUrl)
{
    if (query.host() == baseUrl.host() || (baseUrl.path().length() <= 1 && query.host().contains(QChar('.') + baseUrl.host()))) {
        bool test = query.path().startsWith(baseUrl.path());
        // if (!test) qDebug() << "path mismatch" << query.path() << baseUrl.path();
        return test;
    } else {
        // qDebug() << "host mismatch: " << query.host() << "!=" << baseUrl.host() << baseUrl.path().length() << (QChar('.') + baseUrl.host()) << query.host().contains(QChar('.') + baseUrl.host());
        return false;
    }
}

const int WebCrawler::maxParallelDownloads = 8;
const int WebCrawler::maxVisitedPages = 512 * 16;
