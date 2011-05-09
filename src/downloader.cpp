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

#include "downloader.h"

DownloadJob::DownloadJob(const QUrl &url, const QString &filePattern, Downloader *parent)
    : QObject(parent), m_filePattern(filePattern), m_parent(parent)
{
    qDebug() << "downloading " << url.toString();
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(receivedReply(QNetworkReply *)));
    m_nam->get(QNetworkRequest(url));
}

void DownloadJob::receivedReply(QNetworkReply *reply)
{
    QByteArray data(reply->readAll());
    QString filename = m_filePattern;

    QString md5sum = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    QRegExp md5sumRegExp("%\\{h(:\\d+)?\\}");
    int p = -1;
    while ((p = md5sumRegExp.indexIn(filename)) >= 0) {
        if (md5sumRegExp.cap(1).isEmpty())
            filename = filename.replace(md5sumRegExp.cap(0), md5sum);
        else {
            bool ok = false;
            int left = md5sumRegExp.cap(1).toInt(&ok);
            if (ok && left > 0 && left <= md5sum.length())
                filename = filename.replace(md5sumRegExp.cap(0), md5sum.left(left));
        }
    }

    QString urlString = reply->url().toString().replace(QRegExp("[^a-z0-9]", Qt::CaseInsensitive), "_").replace(QRegExp("_([a-z0-9]{1,4})$", Qt::CaseInsensitive), ".\\1");
    filename = filename.replace("%{s}", urlString);

    QFile output(filename);
    if (output.open(QIODevice::WriteOnly)) {
        output.write(data);
        output.close();

        m_parent->doneDownloading(reply->url(), filename, md5sum);
    }

    m_nam->deleteLater();
    deleteLater();
}


Downloader::Downloader(const QString &filePattern, QTextStream &csvLogFile, QObject *parent)
    : QObject(parent), m_filePattern(filePattern), m_csvLogFile(csvLogFile)
{
    // nothing
}

void Downloader::download(QUrl url)
{
    new DownloadJob(url, m_filePattern, this);
}

void Downloader::doneDownloading(const QUrl &url, const QString &filename, const QString &md5sum)
{
    m_csvLogFile << '"' << md5sum << '"' << ';' << '"' << url.toString() << '"' << ';' << '"' << filename << '"' << endl;
    m_csvLogFile.flush();
    emit downloaded(url, filename);
}