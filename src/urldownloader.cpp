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

#include "urldownloader.h"
#include "watchdog.h"
#include "general.h"

UrlDownloader::UrlDownloader(QNetworkAccessManager *networkAccessManager, const QString &filePattern, QObject *parent)
    : Downloader(parent), m_networkAccessManager(networkAccessManager), m_filePattern(filePattern), m_runningDownloads(0)
{
    // nothing
}

bool UrlDownloader::isAlive()
{
    return m_runningDownloads > 0;
}

void UrlDownloader::download(QUrl url)
{
    /// avoid duplicate URLs
    QString urlString = url.toString();
    if (m_knownUrls.contains(urlString))
        return;
    else
        m_knownUrls.insert(urlString);

    ++m_runningDownloads;

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));

    QTimer *timer = new QTimer(reply);
    m_mapTimerToReply.insert(timer, reply);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
    timer->start(15000);
}

void UrlDownloader::finished()
{
    m_mapTimerToReplyMutex.lock();
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    QTimer *timer = m_mapTimerToReply.key(reply, NULL);
    if (timer != NULL) {
        timer->stop();
        Q_ASSERT(m_mapTimerToReply.remove(timer) == 1);
    }
    m_mapTimerToReplyMutex.unlock();

    bool succeeded = false;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data(reply->readAll());
        QString filename = m_filePattern;

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

        QFileInfo fi(filename);
        if (!fi.absoluteDir().mkpath(fi.absolutePath())) {
            qCritical() << "Cannot create directory" << fi.absolutePath();
        } else {
            QFile output(filename);
            if (output.open(QIODevice::WriteOnly)) {
                output.write(data);
                output.close();

                QString logText = QString("<download url=\"%1\" filename=\"%2\" status=\"success\"/>\n").arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(filename));
                emit downloadReport(logText);
                succeeded = true;

                emit downloaded(reply->url(), filename);
                emit downloaded(filename);

                qDebug() << "Downloaded URL " << reply->url().toString() << " to " << filename;
            }
        }
    }

    if (!succeeded) {
        QString logText = QString("<download url=\"%1\" message=\"download-failed\" detailed=\"%2\" status=\"error\"/>\n").arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(reply->errorString()));
        emit downloadReport(logText);
    }

    QCoreApplication::instance()->processEvents();

    reply->deleteLater();

    --m_runningDownloads;
}

void UrlDownloader::timeout()
{
    m_mapTimerToReplyMutex.lock();
    QTimer *timer = static_cast<QTimer *>(sender());
    QNetworkReply *reply = NULL;
    if (timer != NULL && (reply = m_mapTimerToReply.value(timer)) != NULL) {
        Q_ASSERT(m_mapTimerToReply.remove(timer) == 1);
    } else
        reply = NULL;
    m_mapTimerToReplyMutex.unlock();

    if (reply != NULL) {
        reply->close();
        QString logText = QString("<download url=\"%1\" message=\"timeout\" status=\"error\"/>\n").arg(DocScan::xmlify(reply->url().toString()));
        emit downloadReport(logText);
    }
}
