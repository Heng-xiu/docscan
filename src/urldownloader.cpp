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
#include <QRegExp>
#include <QCryptographicHash>
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSignalMapper>
#include <QMutex>

#include "geoip.h"
#include "urldownloader.h"
#include "watchdog.h"
#include "general.h"


/// various browser strings to "disguise" origin
const QStringList userAgentList = QStringList()
                                  << QLatin1String("Mozilla/5.0 (Linux; U; Android 2.3.3; en-us; HTC_DesireS_S510e Build/GRI40) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1")
                                  << QLatin1String("Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; en-US; rv:1.9.2.3) Gecko/20100402 Prism/1.0b4")
                                  << QLatin1String("Mozilla/5.0 (Windows; U; Win 9x 4.90; SG; rv:1.9.2.4) Gecko/20101104 Netscape/9.1.0285")
                                  << QLatin1String("Mozilla/5.0 (compatible; Konqueror/4.5; FreeBSD) KHTML/4.5.4 (like Gecko)")
                                  << QLatin1String("Mozilla/5.0 (compatible; Yahoo! Slurp China; http://misc.yahoo.com.cn/help.html)")
                                  << QLatin1String("yacybot (x86 Windows XP 5.1; java 1.6.0_12; Europe/de) http://yacy.net/bot.html")
                                  << QLatin1String("Nokia6230i/2.0 (03.25) Profile/MIDP-2.0 Configuration/CLDC-1.1")
                                  << QLatin1String("Links (2.3-pre1; NetBSD 5.0 i386; 96x36)")
                                  << QLatin1String("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/523.15 (KHTML, like Gecko, Safari/419.3) Arora/0.3 (Change: 287 c9dfb30)")
                                  << QLatin1String("Mozilla/4.0 (compatible; Dillo 2.2)")
                                  << QLatin1String("Emacs-W3/4.0pre.46 URL/p4.0pre.46 (i686-pc-linux; X11)")
                                  << QLatin1String("Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.13) Gecko/20080208 Galeon/2.0.4 (2008.1) Firefox/2.0.0.13")
                                  << QLatin1String("Lynx/2.8 (compatible; iCab 2.9.8; Macintosh; U; 68K)")
                                  << QLatin1String("Mozilla/5.0 (Macintosh; U; Intel Mac OS X; en; rv:1.8.1.14) Gecko/20080409 Camino/1.6 (like Firefox/2.0.0.14)")
                                  << QLatin1String("msnbot/2.1")
                                  << QLatin1String("Mozilla/5.0 (iPad; U; CPU OS 3_2 like Mac OS X; en-us) AppleWebKit/531.21.10 (KHTML, like Gecko) Version/4.0.4 Mobile/7B334b Safari/531.21.10")
                                  << QLatin1String("Mozilla/5.0 (Windows; U; ; en-NZ) AppleWebKit/527+ (KHTML, like Gecko, Safari/419.3) Arora/0.8.0")
                                  << QLatin1String("NCSA Mosaic/3.0 (Windows 95)")
                                  << QLatin1String("Mozilla/5.0 (SymbianOS/9.1; U; [en]; Series60/3.0 NokiaE60/4.06.0) AppleWebKit/413 (KHTML, like Gecko) Safari/413")
                                  << QLatin1String("Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US) AppleWebKit/534.16 (KHTML, like Gecko) Chrome/10.0.648.133 Safari/534.16");


UrlDownloader::UrlDownloader(QNetworkAccessManager *networkAccessManager, const QString &filePattern, QObject *parent)
    : Downloader(parent), m_networkAccessManager(networkAccessManager), m_filePattern(filePattern)
{
    m_runningDownloads = m_countSuccessfulDownloads = m_countFaileDownloads = 0;
    m_signalMapperTimeout = new QSignalMapper(this);
    connect(m_signalMapperTimeout, SIGNAL(mapped(QObject *)), this, SLOT(timeout(QObject *)));
    m_setRunningJobs = new QSet<QNetworkReply *>();
    m_internalMutex = new QMutex();
    m_geoip = new GeoIP(m_networkAccessManager);
    qsrand(time(NULL));
    m_userAgent = userAgentList[qrand() % userAgentList.length()];
}

UrlDownloader::~UrlDownloader()
{
    delete m_signalMapperTimeout;
    delete m_setRunningJobs;
    delete m_internalMutex;
    delete m_geoip;
}

bool UrlDownloader::isAlive()
{
    return m_runningDownloads > 0 || m_geoip->isAlive();
}

void UrlDownloader::download(const QUrl &url)
{
    if (url.scheme() == QLatin1String("ftp")) {
        /// Qt 4.7.3 seems to have a bug with FTP downloads
        /// see https://bugreports.qt.nokia.com/browse/QTBUG-22820
        return;
    }

    /// avoid duplicate URLs
    m_internalMutex->lock();
    QString urlString = url.toString();
    if (!m_knownUrls.contains(urlString)) {
        m_knownUrls.insert(urlString);
        m_urlQueue.enqueue(url);
        m_internalMutex->unlock();
        // m_geoip->lookupHost(url.host());
    } else
        m_internalMutex->unlock();

    startNextDownload();
}

void UrlDownloader::startNextDownload()
{
    m_internalMutex->lock();
    if (m_runningDownloads < maxParallelDownloads && !m_urlQueue.isEmpty()) {
        QUrl url = m_urlQueue.dequeue();
        int inQueue = m_urlQueue.count();

        QNetworkRequest request(url);
        request.setRawHeader(QString("User-Agent").toAscii(), m_userAgent.toAscii());
        QNetworkReply *reply = m_networkAccessManager->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(finished()));

        ++m_runningDownloads;
        m_setRunningJobs->insert(reply);

        m_internalMutex->unlock();

        QTimer *timer = new QTimer(reply);
        connect(timer, SIGNAL(timeout()), m_signalMapperTimeout, SLOT(map()));
        m_signalMapperTimeout->setMapping(timer, reply);
        timer->start(15000 + m_runningDownloads * 1000);
        qDebug() << "Downloading " << url.toString() << " (running:" << m_runningDownloads << ", in queue:" << inQueue << ")";
    } else
        m_internalMutex->unlock();
}

void UrlDownloader::finalReport()
{
    QString logText = QString("<download count-success=\"%1\" count-fail=\"%2\">\n").arg(m_countSuccessfulDownloads).arg(m_countFaileDownloads);
    for (QMap<QString, int>::ConstIterator it = m_domainCount.constBegin(); it != m_domainCount.constEnd(); ++it)
        logText += QString(QLatin1String("<domain-count domain=\"%1\" count=\"%2\" />\n")).arg(it.key()).arg(it.value());
    logText += QLatin1String("</download>\n");
    emit report(logText);
}

void UrlDownloader::finished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    m_internalMutex->lock();
    m_setRunningJobs->remove(reply);
    --m_runningDownloads;
    m_internalMutex->unlock();

    bool succeeded = false;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data(reply->readAll());
        QString filename = m_filePattern;

        QString host = reply->url().host().replace(QRegExp("[^.0-9a-z-]", Qt::CaseInsensitive), "X");
        QString domain = QString::null;
        if (domainRegExp.indexIn(host) >= 0) {
            domain = domainRegExp.cap(0);
            filename = filename.replace("%{d}", domain);
        } else
            filename = filename.replace("%{d}", host);

        QString md5sum = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
        QRegExp md5sumRegExp("%\\{h(:(\\d+))?\\}");
        int p = -1;
        while ((p = md5sumRegExp.indexIn(filename)) >= 0) {
            if (md5sumRegExp.cap(1).isEmpty())
                filename = filename.replace(md5sumRegExp.cap(0), md5sum);
            else {
                bool ok = false;
                int left = md5sumRegExp.cap(2).toInt(&ok);
                if (ok && left > 0 && left <= md5sum.length())
                    filename = filename.replace(md5sumRegExp.cap(0), md5sum.left(left));
            }
        }

        QString urlString = reply->url().toString().replace(QRegExp("\\?.*$"), "").replace(QRegExp("[^a-z0-9]", Qt::CaseInsensitive), "_").replace(QRegExp("_([a-z0-9]{1,4})$", Qt::CaseInsensitive), ".\\1");
        filename = filename.replace("%{s}", urlString);

        /// make known file extensions lower-case
        static const QStringList fileExtList = QStringList() << QLatin1String(".pdf") << QLatin1String(".odt") << QLatin1String(".doc") << QLatin1String(".docx") << QLatin1String(".rtf");
        foreach(const QString &fileExt, fileExtList) {
            filename = filename.replace(fileExt, fileExt, Qt::CaseInsensitive);
        }

        if ((p = filename.indexOf("%{")) >= 0)
            qDebug() << "gap was not filled:" << filename.mid(p);

        QFileInfo fi(filename);
        if (!fi.absoluteDir().mkpath(fi.absolutePath())) {
            qCritical() << "Cannot create directory" << fi.absolutePath();
        } else {
            QFile output(filename);
            if (output.open(QIODevice::WriteOnly)) {
                output.write(data);
                output.close();

                if (domain.isEmpty()) {
                    qWarning() << "Domain is empty, host was" << host << "  url was" << reply->url();
                } else {
                    if (m_domainCount.contains(domain))
                        m_domainCount[domain] = m_domainCount[domain] + 1;
                    else
                        m_domainCount[domain] = 1;
                }

                QString geoLocation = QLatin1String(" /");
                // GeoIP::GeoInformation geoInformation = m_geoip->getGeoInformation(reply->url().host());
                // if (!geoInformation.countryCode.isEmpty())
                //    geoLocation = QString(QLatin1String("><geoinformation countrycode=\"%1\" countryname=\"%2\" city=\"%3\" latitude=\"%4\" longitude=\"%5\" /></download")).arg(geoInformation.countryCode).arg(geoInformation.countryName).arg(geoInformation.city).arg(geoInformation.latitude).arg(geoInformation.longitude);

                QString logText = QString("<download url=\"%3\" filename=\"%2\" status=\"success\"%1>\n").arg(geoLocation).arg(DocScan::xmlify(filename)).arg(DocScan::xmlify(reply->url().toString()));

                emit report(logText);
                succeeded = true;

                emit downloaded(reply->url(), filename);
                emit downloaded(filename);

                qDebug() << "Downloaded URL " << reply->url().toString() << " to " << filename << " (running:" << m_runningDownloads << ")";
            }
        }
    }

    if (!succeeded) {
        QString logText = QString("<download url=\"%2\" message=\"download-failed\" detailed=\"%1\" status=\"error\" />\n").arg(DocScan::xmlify(reply->errorString())).arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
        ++m_countFaileDownloads;
    } else
        ++m_countSuccessfulDownloads;

    QCoreApplication::instance()->processEvents();
    reply->deleteLater();
    startNextDownload();
}

void UrlDownloader::timeout(QObject *object)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(object);

    m_internalMutex->lock();
    if (m_setRunningJobs->contains(reply)) {
        m_setRunningJobs->remove(reply);
        m_internalMutex->unlock();
        reply->close();
        QString logText = QString("<download url=\"%1\" message=\"timeout\" status=\"error\" />\n").arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    } else
        m_internalMutex->unlock();
}


const int UrlDownloader::maxParallelDownloads = 8;
const QRegExp UrlDownloader::domainRegExp = QRegExp("[a-z0-9][-a-z0-9]*[a-z0-9]\\.((a[cdefgilmnoqstwxz]|aero|arpa|((com|edu|gob|gov|int|mil|net|org|tur)\\.)?ar|((com|net|org|edu|gov|csiro|asn|id)\\.)?au)|(b[abdefghijmnorstvwyz]|biz)|(c[acdfghiklmnorsuvxyz]|cat|com|coop)|d[ejkmoz]|(e[ceghrstu]|edu)|f[ijkmor]|(g[abdefghilmnpqrstuwy]|gov)|h[kmnrtu]|(i[delmnoqrst]|info|int)|(j[emo]|jobs|((ac|ad|co|ed|go|gr|lg|ne|or)\\.)?jp)|(k[eghimnpwyz]|((co|ne|or|re|pe|go|mil|ac|hs|ms|es|sc|kg)\\.)?kr)|l[abcikrstuvy]|(m[acdghklmnopqrstuvwxyz]|mil|mobi|museum)|(n[acefgilopruz]|name|net)|(om|org)|(p[aefghkmnrstwy]|pro|((com|biz|net|art|edu|org|ngo|gov|info|mil)\\.)?pl)|qa|r[eouw]|s[abcdeghijklmnortvyz]|(t[cdfghjklmnoprtvwz]|travel)|u[agkmsyz]|v[aceginu]|w[fs]|y[etu]|z[amw])$");

