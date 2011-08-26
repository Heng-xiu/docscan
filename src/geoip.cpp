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
#include <QMutex>
#include <QNetworkReply>
#include <QRegExp>

#include "geoip.h"


GeoIP::GeoIP(QNetworkAccessManager *networkAccessManager, QObject *parent)
    : QObject(parent), m_networkAccessManager(networkAccessManager)
{
    m_mutexConcurrentAccess = new QMutex();
    m_numRunningTasks = 0;
}

GeoIP::~GeoIP()
{
    delete m_mutexConcurrentAccess;
}

bool GeoIP::isAlive()
{
    return m_numRunningTasks > 0;
}

void GeoIP::lookupHost(const QString &hostName)
{
    m_mutexConcurrentAccess->lock();
    if (!m_knownHostnames.contains(hostName)) {
        m_knownHostnames.insert(hostName);
        m_mutexConcurrentAccess->unlock();

        ++m_numRunningTasks;
        QHostInfo::lookupHost(hostName, this, SLOT(gotHostInfo(QHostInfo)));
    } else
        m_mutexConcurrentAccess->unlock();
}

QString GeoIP::getCountryCode(const QString &hostName) const
{
    // FIXME: That is active waiting ...
    while (m_numRunningTasks > 0)
        qApp->processEvents();

    m_mutexConcurrentAccess->lock();
    QString result = m_ipAddressToCountryCode.value(m_hostnameToIPaddress.value(hostName, QString::null), QLatin1String("??"));
    m_mutexConcurrentAccess->unlock();
    return result;
}

void GeoIP::gotHostInfo(const QHostInfo &hostInfo)
{
    const QString hostname = hostInfo.hostName();
    if (hostname.isEmpty()) {
        --m_numRunningTasks;
        return;
    }

    QString ipAddress;
    if (!hostInfo.addresses().isEmpty())
        ipAddress = hostInfo.addresses().first().toString();
    if (ipAddress.isEmpty()) {
        --m_numRunningTasks;
        return;
    }

    m_mutexConcurrentAccess->lock();
    m_hostnameToIPaddress.insert(hostname, ipAddress);
    m_mutexConcurrentAccess->unlock();

    m_mutexConcurrentAccess->lock();
    if (!m_ipAddressToCountryCode.contains(ipAddress))       {
        m_mutexConcurrentAccess->unlock();

        ++m_numRunningTasks;

        QUrl url(QString(QLatin1String("http://freegeoip.appspot.com/xml/%1")).arg(ipAddress));
        QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
        connect(reply, SIGNAL(finished()), this, SLOT(finishedGeoInfo()));
        reply->setProperty("ip", ipAddress);

        url = QUrl(QString(QLatin1String("http://api.hostip.info/?ip=%1")).arg(ipAddress));
        reply = m_networkAccessManager->get(QNetworkRequest(url));
        connect(reply, SIGNAL(finished()), this, SLOT(finishedGeoInfo()));
        reply->setProperty("ip", ipAddress);
    } else {
        m_mutexConcurrentAccess->unlock();
        --m_numRunningTasks;
    }
}

void GeoIP::finishedGeoInfo()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        const QString ipAddress = reply->property("ip").toString();
        QTextStream ts(reply->readAll(), QIODevice::ReadOnly);
        const QString xmlText = ts.readAll();

        QString code = "xx";

        if (xmlText.contains(QLatin1String("<Response>"))) {
            if (!xmlText.contains(QLatin1String("<Status>true</Status>")) || xmlText.contains(QLatin1String("<City>Tokyo</City>"))) {
                qDebug() << "Could not resolve location for" << ipAddress << "via freegeoip";
            } else {
                const QRegExp regExpCountryCode(QLatin1String("<CountryCode>(\\S+)</CountryCode>"));
                if (regExpCountryCode.indexIn(xmlText) > 0 && !regExpCountryCode.cap(1).isEmpty())
                    code = regExpCountryCode.cap(1).toLower();
            }
        } else if (xmlText.contains(QLatin1String("<HostipLookupResultSet")) && xmlText.contains(QLatin1String("<Hostip>"))) {
            const QRegExp regExpCountryCode(QLatin1String("<countryAbbrev>(\\S+)</countryAbbrev>"));
            if (regExpCountryCode.indexIn(xmlText) > 0 && !regExpCountryCode.cap(1).isEmpty())
                code = regExpCountryCode.cap(1).toLower();
        } else
            qDebug() << "XML is invalid: " << xmlText;

        if (code != QLatin1String("xx")) {
            if (code == QLatin1String("uk")) code = "gb"; // rewrite United Kingdom to Great Britain
            m_mutexConcurrentAccess->lock();
            m_ipAddressToCountryCode.insert(ipAddress, code);
            m_mutexConcurrentAccess->unlock();
        }
    }

    --m_numRunningTasks;
}
