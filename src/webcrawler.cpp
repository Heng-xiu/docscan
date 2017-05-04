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

#include "webcrawler.h"

#include <QCoreApplication>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QTimer>
#include <QRegExp>
#include <QSignalMapper>
#include <QMutex>
#include <QDebug>

#include "networkaccessmanager.h"
#include "general.h"

static const QStringList blacklistHosts = QStringList() << QStringLiteral("www.nad.riksarkivet.se") << QStringLiteral("nad.riksarkivet.se");

WebCrawler::WebCrawler(NetworkAccessManager *networkAccessManager, const QStringList &filters, const QUrl &baseUrl, const QUrl &startUrl, const QRegExp &requiredContent, int maxVisitedPages, QObject *parent)
    : FileFinder(parent), m_networkAccessManager(networkAccessManager), m_baseUrl(baseUrl.toString()), m_baseHost(QUrl(baseUrl).host()), m_startUrl(startUrl.toString()), m_requiredContent(requiredContent), m_terminating(false), m_shootingNextDownload(false), m_runningDownloads(0)
{
    m_signalMapperTimeout = new QSignalMapper(this);
    connect(m_signalMapperTimeout, SIGNAL(mapped(QObject *)), this, SLOT(timeout(QObject *)));
    m_setRunningJobs = new QSet<QNetworkReply *>();
    m_mutexRunningJobs = new QMutex();
    m_maxParallelDownloads = maxParallelDownloads;
    m_interDownloadDelay = 10; /// milliseconds
    m_maxVisitedPages = qMin(maxVisitedPages, WebCrawler::maxVisitedPages);

    foreach(const QString &label, filters) {
        Filter filter;
        filter.label = label;
        filter.foundHits = 0;
        QString rpl = label;
        rpl = rpl.replace(QStringLiteral("."), QStringLiteral("\\.")).replace(QStringLiteral("?"), QStringLiteral(".")).replace(QStringLiteral("*"), QStringLiteral("[^/ \"']*"));
        filter.regExp = QRegExp(QString(QStringLiteral("(^|/)(%1)([?].+)?$")).arg(rpl));
        m_filterSet.append(filter);
    }
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
    if (m_startUrl.contains(QStringLiteral("transportstyrelsen.se"))) {
        /// Transportstyrelsen has a download-per-time limit
        m_maxParallelDownloads = 1;
        m_interDownloadDelay = 1750; /// milliseconds
    }
    m_visitedPages = 0;
    QStringList regExpList;
    for (QList<Filter>::Iterator it = m_filterSet.begin(); it != m_filterSet.end(); ++it) {
        it->foundHits = 0;
        regExpList << it->regExp.pattern();
    }

    m_queuedUrls.clear();
    m_queuedUrls << m_startUrl;
    m_knownUrls.clear();
    m_knownUrls << m_startUrl;

    emit report(QString(QStringLiteral("<webcrawler numexpectedhits=\"%2\"><filepattern>%1</filepattern></webcrawler>\n")).arg(DocScan::xmlify(regExpList.join(QChar('|')))).arg(m_numExpectedHits));

    visitNextPage();
}

bool WebCrawler::isAlive()
{
    return !m_terminating || m_shootingNextDownload || m_runningDownloads > 0;
}

bool WebCrawler::visitNextPage()
{
    int startedDownloads = 0;

    bool numExpectedHitsReached = true;
    for (QList<Filter>::ConstIterator it = m_filterSet.constBegin(); it != m_filterSet.constEnd(); ++it) {
        numExpectedHitsReached &= it->foundHits >= m_numExpectedHits;
    }
    if (numExpectedHitsReached) {
        emit report(QString(QStringLiteral("<webcrawler numexpectedhitsreached=\"%1\" />\n")).arg(m_numExpectedHits));
        return false;
    }

    while (m_runningDownloads < m_maxParallelDownloads) {
        if (m_runningDownloads >= m_maxParallelDownloads || m_queuedUrls.isEmpty())
            break;
        const QString url = m_queuedUrls.first();
        m_queuedUrls.removeFirst();

        QNetworkRequest request = QNetworkRequest(QUrl(url));
        qDebug() << "Crawling page on " << url << "host: " << request.url().host() << "(" << m_visitedPages << ")";

        if (blacklistHosts.contains(request.url().host())) {
            qDebug() << "Skipping blacklisted host" << request.url().host();
            continue;
        }

        ++m_visitedPages;
        if (m_visitedPages > m_maxVisitedPages)
            break;

        ++m_runningDownloads;
        ++startedDownloads;

        /*
         * FIXME neccessary?
        /// Enable TLSv1
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setProtocol(QSsl::TlsV1);
        request.setSslConfiguration(config);
        */

        QNetworkReply *reply = m_networkAccessManager->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(finishedDownload()));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(gotSslErrors(QList<QSslError>)));

        m_mutexRunningJobs->lock();
        m_setRunningJobs->insert(reply);
        m_mutexRunningJobs->unlock();
        QTimer *timer = new QTimer(reply);
        connect(timer, SIGNAL(timeout()), m_signalMapperTimeout, SLOT(map()));
        m_signalMapperTimeout->setMapping(timer, reply);
        timer->start(10000 + m_runningDownloads * 1000);
    }

    return startedDownloads > 0;
}

void WebCrawler::finishedDownload()
{
    QRegExp fileExtRegExp = QRegExp(QStringLiteral("[.].{1,4}$"), Qt::CaseInsensitive);
    QRegExp validFileExtRegExp = QRegExp(QStringLiteral("([.]([sp]?htm[l]?|jsp|asp[x]?|php)|[^.]{5,})([?].+)?$"), Qt::CaseInsensitive);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    m_mutexRunningJobs->lock();
    m_setRunningJobs->remove(reply);
    m_mutexRunningJobs->unlock();

    /// check for redirections
    QString redirUrlStr = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
    if (!redirUrlStr.isEmpty()) {
        QUrl redirUrl = reply->url().resolved(QUrl(redirUrlStr));
        if (redirUrl.isValid() && !m_knownUrls.contains((redirUrlStr = redirUrl.toString())) && !blacklistHosts.contains(redirUrl.host())) {
            m_knownUrls << redirUrlStr;
            m_queuedUrls << redirUrlStr;
        }
    } else if (reply->error() == QNetworkReply::HostNotFoundError) {
        /// schwaebischhall.de does not resolve, but www.schwaebischhall.de does
        QString hostname = reply->url().host();
        if (hostname.startsWith(QStringLiteral("www.")))
            /// try removing leading "www."
            hostname = hostname.mid(4);
        else
            /// try prepending missing "www."
            hostname = hostname.prepend(QStringLiteral("www."));
        QUrl redirUrl = reply->url();
        redirUrl.setHost(hostname);

        if (!blacklistHosts.contains(hostname)) {
            const QString redirUrlStr = redirUrl.toString();
            m_knownUrls << redirUrlStr;
            m_queuedUrls << redirUrlStr;
        }
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data(reply->readAll());
        QString text(data);

        /// check if HTML page ...
        if (text.left(256).contains(QStringLiteral("<html"), Qt::CaseInsensitive)) {
            emit report(QString(QStringLiteral("<webcrawler status=\"success\" url=\"%1\" />\n")).arg(DocScan::xmlify(reply->url().toString())));
            if (m_requiredContent.isEmpty() || text.contains(m_requiredContent)) {

                /// collect hits
                QSet<QString> hitCollection;

                /// for each anchor in the HTML code
                QRegExp anchorRegExp("<a\\b[^>]*href=[\"']?([^'\" \t><]+)", Qt::CaseInsensitive);
                int p = -1;
                while ((p = text.indexOf(anchorRegExp, p + 1)) >= 0) {
                    const QUrl url = normalizeUrl(anchorRegExp.cap(1), reply->url());
                    const QString urlStr = url.toString();

                    if (url.isEmpty()) continue;
                    if (m_knownUrls.contains(urlStr)) continue;

                    /// simplification: extension (with or without dot) is four chars long
                    QString extension = urlStr.right(4).toLower();
                    /// exclude images
                    if (extension == QStringLiteral(".jpg") || extension == QStringLiteral("jpeg") || extension == QStringLiteral(".png") || extension == QStringLiteral(".gif") || extension == QStringLiteral(".eps") || extension == QStringLiteral(".bmp"))
                        continue;
                    /// exclude multimedia files
                    if (extension == QStringLiteral(".avi") || extension == QStringLiteral("mpeg") || extension == QStringLiteral(".mpg") || extension == QStringLiteral(".mp4") || extension == QStringLiteral(".mp3") || extension == QStringLiteral(".wmv") || extension == QStringLiteral(".wma"))
                        continue;
                    /// exclude Microsoft Office files
                    //if (extension == QStringLiteral(".doc") || extension == QStringLiteral("docx") || extension == QStringLiteral(".ppt") || extension == QStringLiteral("pptx") || extension == QStringLiteral(".xls") || extension == QStringLiteral("xlsx"))
                    //    continue;
                    /// only files from domain
                    if (!url.host().endsWith(m_baseHost))
                        continue;

                    m_knownUrls << urlStr;

                    bool regExpMatches = false;
                    for (QList<Filter>::Iterator it = m_filterSet.begin(); it != m_filterSet.end(); ++it) {
                        if (it->regExp.indexIn(urlStr) >= 0) {
                            /// link matches requested file type
                            regExpMatches = true;
                            it->foundHits += 1;
                            break;
                        }
                    }

                    if (regExpMatches) {
                        emit report(QString(QStringLiteral("<webcrawler detailed=\"Found regexp match\" status=\"success\" url=\"%1\" href=\"%2\" />\n")).arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(urlStr)));
                        hitCollection.insert(urlStr);
                    } else if (!isSubAddress(QUrl(url), QUrl(m_baseUrl))) {
                        // qDebug() << "Is not a sub-address:" << urlStr << "of" << m_baseUrl;
                    } else if (validFileExtRegExp.indexIn(urlStr) == 0) {
                        // qDebug() << "Path or extension is not wanted" << urlStr;
                    } else if (!blacklistHosts.contains(url.host()) && !blacklistHosts.contains(reply->url().host())) {
                        // emit report(QString(QStringLiteral("<webcrawler detailed=\"Found follow-up link\" status=\"success\" url=\"%1\" href=\"%2\" />\n")).arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(urlStr)));
                        m_queuedUrls << urlStr;
                    }
                }

                /// delay sending signals to ensure BFS on links
                foreach(const QString &url, hitCollection) {
                    emit report(QString(QStringLiteral("<filefinder event=\"hit\" href=\"%1\" />\n")).arg(DocScan::xmlify(url)));
                    emit foundUrl(QUrl(url));
                }
            } else
                emit report(QString(QStringLiteral("<webcrawler detailed=\"Required content not found\" status=\"error\" url=\"%1\" />\n")).arg(DocScan::xmlify(reply->url().toString())));
        } else if (text.startsWith(QStringLiteral("%PDF-1."))) {
            const QString urlStr = reply->url().toString();
            bool regExpLookingForPDF = false;
            QList<Filter>::Iterator it = m_filterSet.begin();
            for (; it != m_filterSet.end(); ++it) {
                regExpLookingForPDF = it->regExp.pattern().contains(QStringLiteral(".pdf"));
                if (regExpLookingForPDF) break;
            }
            if (regExpLookingForPDF && it != m_filterSet.end()) {
                /// Found an URL that does not match regular expression, but points
                /// to an PDF file which the regular expression is looking for.
                /// So, keep this URL...
                it->foundHits += 1; ///< count as hit
                emit report(QString(QStringLiteral("<webcrawler detailed=\"Found URL pointing to PDF\" status=\"success\" url=\"%1\" />\n")).arg(DocScan::xmlify(urlStr)));
                emit report(QString(QStringLiteral("<filefinder event=\"hit\" href=\"%1\" />\n")).arg(DocScan::xmlify(urlStr)));
                emit foundUrl(reply->url());
            } else
                emit report(QString(QStringLiteral("<webcrawler detailed=\"Found a PDF, but not looking for such files\" status=\"error\" url=\"%1\" />\n")).arg(DocScan::xmlify(reply->url().toString())));
        } else
            emit report(QString(QStringLiteral("<webcrawler detailed=\"Not an HTML page\" status=\"error\" url=\"%1\">%2</webcrawler>\n")).arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(text.left(32))));
    } else
        emit report(QString(QStringLiteral("<webcrawler detailed=\"%2\" status=\"error\" code=\"%3\" url=\"%1\" />\n")).arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(reply->errorString())).arg(reply->error()));

    qApp->processEvents();
    --m_runningDownloads;

    m_shootingNextDownload = true;
    QTimer::singleShot(m_interDownloadDelay, this, SLOT(singleShotNextDownload()));

    reply->deleteLater();
}

void WebCrawler::gotSslErrors(const QList<QSslError> &list)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    /// Log all SSL/TLS errors
    foreach(const QSslError &error, list) {
        const QString logText = QString(QStringLiteral("<webcrawler detailed=\"SSL/TLS: %1\" status=\"warning\" url=\"%2\" />\n")).arg(DocScan::xmlify(error.errorString())).arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
        qWarning() << "Ignoring SSL error: " << error.errorString();
    }
    /// Ignore all SSL/TLS errors
    reply->ignoreSslErrors(list);
}

void WebCrawler::singleShotNextDownload()
{
    m_shootingNextDownload = false;
    bool downloadStarted = visitNextPage();

    if (!downloadStarted && m_runningDownloads == 0) {
        if (!m_terminating) {
            /// web crawler has stopped as there are no downloads active
            QString reportStr = QString(QStringLiteral("<webcrawler maxvisitedpages=\"%4\" numexpectedhits=\"%1\" numknownurls=\"%2\" numvisitedpages=\"%3\" numqueuedurls=\"%5\">\n")).arg(m_numExpectedHits).arg(m_knownUrls.count()).arg(m_visitedPages).arg(m_maxVisitedPages).arg(m_queuedUrls.count());
            for (QList<Filter>::ConstIterator it = m_filterSet.constBegin(); it != m_filterSet.constEnd(); ++it) {
                reportStr += QString(QStringLiteral("<filter numfoundhits=\"%1\" pattern=\"%2\" />\n")).arg(it->foundHits).arg(DocScan::xmlify(it->label));
            }
            reportStr += QStringLiteral("</webcrawler>\n");
            emit report(reportStr);
            m_terminating = true;
        } else {
            /// This should never happen
            qWarning() << "downloadStarted=" << downloadStarted << "  m_runningDownloads=" << m_runningDownloads << "  m_terminating=" << m_terminating;
        }
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
        QString logText = QString(QStringLiteral("<download message=\"timeout\" status=\"error\" url=\"%1\" />\n")).arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    } else
        m_mutexRunningJobs->unlock();
}

QUrl WebCrawler::normalizeUrl(const QString &partialUrl, const QUrl &baseUrl) const
{
    /// Ignore "mailto:" links
    if (partialUrl.contains(QStringLiteral("mailto:")))
        return QUrl();

    /// Fix some HTML-encodings
    QString text = partialUrl;
    text.replace(QStringLiteral("&amp;"), QChar('&'));

    /// complete URL to absolute URL
    QUrl urlObj = baseUrl.resolved(QUrl::fromUserInput(text));
    if (urlObj.path().isEmpty()) urlObj.setPath(QChar('/'));

    /// URL has to start with "http"
    if (!urlObj.scheme().startsWith(QStringLiteral("http")))
        return QUrl();

    /// remove JavaScript links or links pointed in page-internal anchors
    urlObj.setFragment(QString::null);

    return urlObj;
}

bool WebCrawler::isSubAddress(const QUrl &query, const QUrl &baseUrl)
{
    if (query.host() == baseUrl.host() || (baseUrl.path().length() <= 1 && query.host().contains(QChar('.') + baseUrl.host()))) {
        bool test = query.path().startsWith(baseUrl.path());
        return test;
    } else {
        return false;
    }
}

const int WebCrawler::maxParallelDownloads = 16;
const int WebCrawler::maxVisitedPages = 1048576;
