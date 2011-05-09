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

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QUrl>

class QNetworkReply;
class QNetworkAccessManager;
class QTextStream;

class Downloader;

class DownloadJob : public QObject
{
    Q_OBJECT

public:
    DownloadJob(const QUrl &url, const QString &filePattern, Downloader *parent);

private:
    QNetworkAccessManager *m_nam;
    const QString m_filePattern;
    Downloader *m_parent;

private slots:
    void receivedReply(QNetworkReply *reply);
};

class Downloader : public QObject
{
    Q_OBJECT
public:
    explicit Downloader(const QString &filePattern, QTextStream &csvLogFile, QObject *parent = 0);

    friend class DownloadJob;

public slots:
    void download(QUrl);

signals:
    void downloaded(QUrl, QString);

private:
    const QString m_filePattern;
    QTextStream &m_csvLogFile;

    void doneDownloading(const QUrl &url, const QString &filename, const QString &md5sum);
};

#endif // DOWNLOADER_H