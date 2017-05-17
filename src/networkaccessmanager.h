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

#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>
#include <QStringList>
#include <QNetworkCookieJar>

class CookieJar : public QNetworkCookieJar
{
    Q_OBJECT

public:
    CookieJar(QObject *parent = nullptr);
};

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    NetworkAccessManager(QObject *parent = nullptr);

    void setRequestHeaders(QNetworkRequest &request);

private:
    const QStringList m_userAgentList;
    QString m_userAgent;
};

#endif // NETWORKACCESSMANAGER_H
