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

#ifndef URLDOWNLOADER_H
#define URLDOWNLOADER_H

#include <QUrl>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <QMutex>

#include "downloader.h"

class QNetworkAccessManager;
class QNetworkReply;

class UrlDownloader : public Downloader
{
    Q_OBJECT
public:
    explicit UrlDownloader(QNetworkAccessManager *networkAccessManager, const QString &filePattern, QObject *parent = 0);

    virtual bool isAlive();

    friend class DownloadJob;

public slots:
    void download(QUrl);

signals:
    void downloaded(QUrl, QString);
    void downloaded(QString);
    void downloadReport(QString);

private:
    QNetworkAccessManager *m_networkAccessManager;
    const QString m_filePattern;
    int m_runningDownloads;
    QMap<QTimer *, QNetworkReply *> m_mapTimerToReply;
    QMutex m_mapTimerToReplyMutex;
    QSet<QString> m_knownUrls;
    static const QRegExp domainRegExp;

private slots:
    void finished();
    void timeout();
};

#endif // URLDOWNLOADER_H
