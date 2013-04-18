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
#include "networkaccessmanager.h"

UrlDownloader::UrlDownloader(NetworkAccessManager *networkAccessManager, const QString &filePattern, int maxDownloads, QObject *parent)
    : Downloader(parent), m_networkAccessManager(networkAccessManager), m_filePattern(filePattern), m_maxDownloads(maxDownloads)
{
    m_runningDownloads = m_countSuccessfulDownloads = m_countFailedDownloads = 0;
    m_runningdownloadsPerHostname.clear();
    m_signalMapperTimeout = new QSignalMapper(this);
    connect(m_signalMapperTimeout, SIGNAL(mapped(QObject *)), this, SLOT(timeout(QObject *)));
    m_setRunningJobs = new QSet<QNetworkReply *>();
    m_internalMutex = new QMutex();
    m_geoip = new GeoIP(m_networkAccessManager);
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
        qDebug() << "FTP protocol not supported for URL " << url.toString();
        return;
    }

    if (m_countSuccessfulDownloads > m_maxDownloads) {
        /// already reached limit of maximum downloads
        qDebug() << "Reached maximum download of " << m_maxDownloads << " > " << m_countSuccessfulDownloads << " for URL " << url.toString();
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
    if (m_runningDownloads < maxParallelDownloads && !m_urlQueue.isEmpty() && m_runningdownloadsPerHostname.value(m_urlQueue.first().host(), 0) < maxParallelDownloadsPerHost) {
        QUrl url = m_urlQueue.dequeue();
        int inQueue = m_urlQueue.count();

        QNetworkRequest request(url);
        m_networkAccessManager->setRequestHeaders(request);
        QNetworkReply *reply = m_networkAccessManager->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(finished()));

        ++m_runningDownloads;
        const QString hostname = url.host();
        m_runningdownloadsPerHostname[hostname] = m_runningdownloadsPerHostname.value(hostname, 0) + 1;
        m_setRunningJobs->insert(reply);

        m_internalMutex->unlock();

        QTimer *timer = new QTimer(reply);
        connect(timer, SIGNAL(timeout()), m_signalMapperTimeout, SLOT(map()));
        m_signalMapperTimeout->setMapping(timer, reply);
        timer->start(15000 + m_runningDownloads * 1000);
        qDebug() << "Downloading " << url.toString() << " (running:" << m_runningDownloads << ", per host running:" << m_runningdownloadsPerHostname[hostname] << ", in queue:" << inQueue << ")";
    } else
        m_internalMutex->unlock();
}

void UrlDownloader::finalReport()
{
    QString logText = QString("<download count-fail=\"%2\" count-success=\"%1\">\n").arg(m_countSuccessfulDownloads).arg(m_countFailedDownloads);
    for (QMap<QString, int>::ConstIterator it = m_domainCount.constBegin(); it != m_domainCount.constEnd(); ++it)
        logText += QString(QLatin1String("<domain-count count=\"%2\" domain=\"%1\" />\n")).arg(it.key()).arg(it.value());
    logText += QLatin1String("</download>\n");
    emit report(logText);
}

void ensureExtension(QString &filename, const QString &extension)
{
    if (filename.isEmpty() || extension.isEmpty())
        return;
    const QString internalExt = extension.startsWith(QLatin1Char('.')) ? extension : QLatin1Char('.') + extension;
    if (!filename.endsWith(internalExt, Qt::CaseInsensitive))
        filename = filename.append(internalExt);
}

void UrlDownloader::finished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    m_internalMutex->lock();
    m_setRunningJobs->remove(reply);
    --m_runningDownloads;
    const QString hostname = reply->url().host();
    m_runningdownloadsPerHostname[hostname] = m_runningdownloadsPerHostname.value(hostname, 1) - 1;
    m_internalMutex->unlock();

    bool succeeded = false;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data(reply->readAll());
        QString filename = m_filePattern;

        const QString host = reply->url().host().replace(QRegExp("[^.0-9a-z-]", Qt::CaseInsensitive), "X");
        QString domain = QString::null;
        if (domainRegExp.indexIn(host) >= 0) {
            domain = domainRegExp.cap(0);
            filename = filename.replace("%{d}", domain);
        } else
            filename = filename.replace("%{d}", host);

        static const QRegExp dateTimeRegExp("%\\{D:([-_%a-zA-Z0-9]+)\\}");
        int p = -1;
        QDateTime dateTime = QDateTime::currentDateTime();
        while ((p = dateTimeRegExp.indexIn(filename)) >= 0) {
            QString dateTimeStr = dateTime.toString(dateTimeRegExp.cap(1));
            /// support "ww" for zero-padded, two-digit week numbers
            dateTimeStr = dateTimeStr.replace(QLatin1String("ww"), QLatin1String("%1"));
            if (dateTimeStr.contains(QLatin1String("%1")))
                dateTimeStr = dateTimeStr.arg(dateTime.date().weekNumber(), 2, 10, QChar('0'));
            /// support "w" for plain week numbers (one or two digits)
            dateTimeStr = dateTimeStr.replace(QLatin1String("ww"), QString::number(dateTime.date().weekNumber()));
            /// support "DDD" for zero-padded, three-digit day-of-the-year numbers
            dateTimeStr = dateTimeStr.replace(QLatin1String("DDD"), QLatin1String("%1"));
            if (dateTimeStr.contains(QLatin1String("%1")))
                dateTimeStr = dateTimeStr.arg(dateTime.date().dayOfYear(), 3, 10, QChar('0'));
            /// support "D" for plain day-of-the-year numbers (one, two, or three digits)
            dateTimeStr = dateTimeStr.replace(QLatin1String("D"), QString::number(dateTime.date().dayOfYear()));
            /// insert date/time into filename
            filename = filename.replace(dateTimeRegExp.cap(0), dateTimeStr);
        }

        QString md5sum = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
        static const QRegExp md5sumRegExp("%\\{h(:(\\d+))?\\}");
        p = -1;
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
        /// enforce .pdf file name extension if not exists but file name contains "pdf"
        if (urlString.contains(QLatin1String("pdf"), Qt::CaseInsensitive) && !urlString.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive))
            urlString = urlString.append(QLatin1String(".pdf"));

        static const QRegExp fileExtensionRegExp(QLatin1String("[.](.{2,4})([?].+)?$"));
        QString fileExtension = QString::null;
        if (fileExtensionRegExp.indexIn(filename) == 0 || (fileExtension = fileExtensionRegExp.cap(1)).isEmpty()) {
            const QString url = reply->url().toString().toLower();

            /// Filename has no extension, so test data which extension would be fitting
            if ((data[0] == '%' && data[1] == 'P' && data[2] == 'D' && data[3] == 'F') || url.contains(QLatin1String("application/pdf")) || url.contains(QLatin1String(".pdf"))) {
                fileExtension = QLatin1String("pdf");
            } else if ((data[0] == '\\' && data[1] == '{' && data[2] == 'r' && data[3] == 't' && data[4] == 'f') || url.contains(QLatin1String(".rtf"))) {
                fileExtension = QLatin1String("rtf");
            } else if (url.contains(QLatin1String(".odt"))) {
                /// Open Document Format text
                fileExtension = QLatin1String("odt");
            } else if (url.contains(QLatin1String(".ods"))) {
                /// Open Document Format spreadsheet
                fileExtension = QLatin1String("ods");
            } else if (url.contains(QLatin1String(".odp"))) {
                /// Open Document Format spreadsheet
                fileExtension = QLatin1String("odp");
            } else if (url.contains(QLatin1String(".docx"))) {
                /// Office Open XML text
                fileExtension = QLatin1String("docx");
            } else if (url.contains(QLatin1String(".pptx"))) {
                /// Office Open XML presentation
                fileExtension = QLatin1String("pptx");
            } else if (url.contains(QLatin1String(".xlsx"))) {
                /// Office Open XML spreadsheet
                fileExtension = QLatin1String("xlsx");
            } else if (url.contains(QLatin1String(".doc"))) {
                /// archaic .doc
                fileExtension = QLatin1String("doc");
            } else if (url.contains(QLatin1String(".ppt"))) {
                /// archaic .ppt
                fileExtension = QLatin1String("ppt");
            } else if (url.contains(QLatin1String(".xls"))) {
                /// archaic .xls
                fileExtension = QLatin1String("xls");
            } else if ((data[0] == (char)0xd0 && data[1] == (char)0xcf && data[2] == (char)0x11) || (url.contains(QLatin1String(".doc")) && !url.contains(QLatin1String(".docx")))) {
                /// some kind of archaic Microsoft format, assuming .doc as most popular
                fileExtension = QLatin1String("doc");
            } else if ((data[0] == (char)'P' && data[1] == (char)'K' && (int)data[2] < 10) || url.contains(QLatin1String(".zip"))) {
                /// .zip file, could be ODF or 00XML (further testing required)
                fileExtension = QLatin1String("zip");
            }
        }
        filename = filename.replace("%{x}", fileExtension);
        ensureExtension(filename, fileExtension);

        if ((p = filename.indexOf("%{")) >= 0)
            qWarning() << "gap was not filled:" << filename.mid(p);

        QFileInfo fi(filename);
        if (!fi.absoluteDir().mkpath(fi.absolutePath())) {
            qCritical() << "Cannot create directory" << fi.absolutePath();
        } else {
            QFile output(filename);
            if (output.open(QIODevice::WriteOnly)) {
                output.write(data);
                output.close();

                if (domain.isEmpty()) {
                    if (!reply->url().isLocalFile())
                        qWarning() << "Domain is empty, host was" << host << "  url was" << reply->url();
                } else {
                    if (m_domainCount.contains(domain))
                        m_domainCount[domain] = m_domainCount[domain] + 1;
                    else
                        m_domainCount[domain] = 1;
                }

                DocScan::XMLNode geoLocationNode;
                GeoIP::GeoInformation geoInformation = m_geoip->getGeoInformation(reply->url().host());
                if (!geoInformation.countryCode.isEmpty()) {
                    geoLocationNode.name = QLatin1String("geoinformation");
                    geoLocationNode.attributes.insert(QLatin1String("countrycode"), geoInformation.countryCode);
                    geoLocationNode.attributes.insert(QLatin1String("countryname"), geoInformation.countryName);
                    geoLocationNode.attributes.insert(QLatin1String("city"), geoInformation.city);
                    geoLocationNode.attributes.insert(QLatin1String("latitude"), QString::number(geoInformation.latitude));
                    geoLocationNode.attributes.insert(QLatin1String("longitude"), QString::number(geoInformation.longitude));
                }

                DocScan::XMLNode logNode;
                logNode.name = QLatin1String("download");
                logNode.attributes.insert(QLatin1String("url"), reply->url().toString());
                logNode.attributes.insert(QLatin1String("filename"), filename);
                logNode.attributes.insert(QLatin1String("status"), QLatin1String("success"));
                if (!domain.isEmpty())
                    logNode.attributes.insert(QLatin1String("domain"), domain);
                if (!geoLocationNode.name.isEmpty())
                    logNode.text += DocScan::xmlNodeToText(geoLocationNode);
                emit report(DocScan::xmlNodeToText(logNode));

                succeeded = true;

                emit downloaded(reply->url(), filename);
                emit downloaded(filename);

                qDebug() << "Downloaded URL " << reply->url().toString() << " to " << filename << " (running:" << m_runningDownloads << ")";
            }
        }
    }

    if (!succeeded) {
        QString logText = QString("<download detailed=\"%1\" message=\"download-failed\" status=\"error\" url=\"%2\" />\n").arg(DocScan::xmlify(reply->errorString())).arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
        ++m_countFailedDownloads;
    } else
        ++m_countSuccessfulDownloads;

    QCoreApplication::instance()->processEvents();
    reply->deleteLater();
    startNextDownload();
}

void UrlDownloader::timeout(QObject *object)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(object);

    m_internalMutex->lock();
    if (m_setRunningJobs->contains(reply)) {
        m_setRunningJobs->remove(reply);
        m_internalMutex->unlock();
        reply->close();
        QString logText = QString("<download message=\"timeout\" status=\"error\" url=\"%1\" />\n").arg(DocScan::xmlify(reply->url().toString()));
        emit report(logText);
    } else
        m_internalMutex->unlock();
}


const int UrlDownloader::maxParallelDownloads = 16;
const int UrlDownloader::maxParallelDownloadsPerHost = 4;
const QRegExp UrlDownloader::domainRegExp = QRegExp("[a-z0-9][-a-z0-9]*[a-z0-9]\\.((a[cdefgilmnoqstwxz]|aero|arpa|((com|edu|gob|gov|int|mil|net|org|tur)\\.)?ar|((com|net|org|edu|gov|csiro|asn|id)\\.)?au)|(((adm|adv|agr|am|arq|art|ato|b|bio|blog|bmd|cim|cng|cnt|com|coop|ecn|edu|eng|esp|etc|eti|far|flog|fm|fnd|fot|fst||ggf|gov|imb|ind|inf|jor|jus|lel|mat|med|mil|mus|net|nom|not|ntr|odo|org|ppg|pro|psc|psi|qsl|radio|rec|slg|srv|taxi|teo|tmp|trd|tur|tv|vet|vlog|wiki|zlg)\\.)?br|b[abdefghijmnorstvwyz]|biz)|(((ab|bc|mb|nb|nf|nl|ns|nt|nu|on|pe|qc|sk|yk)\\.)?ca|c[cdfghiklmnorsuvxyz]|cat|com|coop)|d[ejkmoz]|(e[ceghrstu]|edu)|f[ijkmor]|(g[abdefghilmnpqrstuwy]|gov)|h[kmnrtu]|(i[delmnoqrst]|info|int)|(j[emo]|jobs|((ac|ad|co|ed|go|gr|lg|ne|or)\\.)?jp)|(k[eghimnpwyz]|((co|ne|or|re|pe|go|mil|ac|hs|ms|es|sc|kg)\\.)?kr)|l[abcikrstuvy]|(m[acdghklmnopqrstuvwxyz]|mil|mobi|museum)|(n[acefgilopruz]|name|net)|(om|org)|(p[aefghkmnrstwy]|pro|((com|biz|net|art|edu|org|ngo|gov|info|mil)\\.)?pl)|qa|r[eouw]|s[abcdeghijklmnortvyz]|(t[cdfghjklmnoprtvwz]|travel)|((ac|co|gov|ltd|me|mod|org|sch)\\.uk)|(((dni|fed|isa|kids|nsn|ak|al|ar|as|az|ca|co|ct|dc|de|fl|ga|gu|hi|ia|id|il|in|ks|ky|la|ma|md|me|mi|mn|mo|mp|ms|mt|nc|nd|ne|nh|nj|nm|nv|ny|oh|ok|or|pa|pr|ri|sc|sd|tn|tx|um|ut|va|vi|vt|wa|wi|wv|wy)\\.)?us|u[agmyz])|v[aceginu]|w[fs]|y[etu]|z[amw])$");

