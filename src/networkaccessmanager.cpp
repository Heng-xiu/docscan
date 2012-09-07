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

#include <QNetworkRequest>
#include <QNetworkCookieJar>
#include <QDebug>

#include "networkaccessmanager.h"

class CookieJar : public QNetworkCookieJar
{
public:
    CookieJar(QObject *parent = 0)
        : QNetworkCookieJar(parent) {
        // TODO
    }

};

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent), m_userAgentList(QStringList() << QLatin1String("Mozilla/5.0 (X11; Linux i686; rv:10.0.6) Gecko/20100101 Firefox/10.0.6") << QLatin1String("Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:14.0) Gecko/20100101 Firefox/14.0.1"))
{
    setCookieJar(new CookieJar(this));
    qsrand(time(NULL));
    m_userAgent = m_userAgentList[qrand() % m_userAgentList.length()];
    qDebug() << "Using user agent" << m_userAgent;
}

void NetworkAccessManager::setRequestHeaders(QNetworkRequest &request)
{
    request.setRawHeader("User-Agent", m_userAgent.toAscii());
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,text/*;q=0.8,*/*;q=0.6");
}
