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

GeoIP::GeoInformation GeoIP::getGeoInformation(const QString &hostName) const
{
    // FIXME: That is active waiting ...
    while (m_numRunningTasks > 0)
        qApp->processEvents();

    m_mutexConcurrentAccess->lock();
    GeoInformation result = m_ipAddressToGeoInformation.value(m_hostnameToIPaddress.value(hostName, QString::null), GeoInformation());
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
    if (!m_ipAddressToGeoInformation.contains(ipAddress))       {
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

        GeoInformation geoInformation;

        if (xmlText.contains(QLatin1String("<Response>"))) {
            if (!xmlText.contains(QLatin1String("<Status>true</Status>")) || xmlText.contains(QLatin1String("<City>Tokyo</City>"))) {
                // qDebug() << "Could not resolve location for" << ipAddress << "via freegeoip";
            } else {
                const QRegExp regExpCountryCode(QLatin1String("<CountryCode>(\\S+)</CountryCode>"));
                if (regExpCountryCode.indexIn(xmlText) > 0 && !regExpCountryCode.cap(1).isEmpty())
                    geoInformation.countryCode = regExpCountryCode.cap(1).toLower();
                const QRegExp regExpCountryName(QLatin1String("<CountryName>([^<]+)</CountryName>"));
                if (regExpCountryName.indexIn(xmlText) > 0 && !regExpCountryName.cap(1).isEmpty())
                    geoInformation.countryName = regExpCountryName.cap(1).toLower();
                const QRegExp regExpCity(QLatin1String("<City>([^<]+)</City>"));
                if (regExpCity.indexIn(xmlText) > 0 && !regExpCity.cap(1).isEmpty() && regExpCity.cap(1).contains("unknown"))
                    geoInformation.city = regExpCity.cap(1).toLower();
                const QRegExp regExpLatitude(QLatin1String("<Latitude>([-0-9.]+)</Latitude>"));
                bool ok = false;
                if (!regExpLatitude.indexIn(xmlText) < 0 || regExpLatitude.cap(1).isEmpty() || (geoInformation.latitude = regExpLatitude.cap(1).toFloat(&ok)) > 1000.0 || !ok)
                    geoInformation.latitude = 0.0;
                const QRegExp regExpLongitude(QLatin1String("<Longitude>([-0-9.]+)</Longitude>"));
                ok = false;
                if (!regExpLongitude.indexIn(xmlText) < 0 || regExpLongitude.cap(1).isEmpty() || (geoInformation.longitude = regExpLongitude.cap(1).toFloat(&ok)) > 1000.0 || !ok)
                    geoInformation.longitude = 0.0;
            }
        } else if (xmlText.contains(QLatin1String("<HostipLookupResultSet")) && xmlText.contains(QLatin1String("<Hostip>"))) {
            const QRegExp regExpCountryCode(QLatin1String("<countryAbbrev>(\\S+)</countryAbbrev>"));
            if (regExpCountryCode.indexIn(xmlText) > 0 && !regExpCountryCode.cap(1).isEmpty())
                geoInformation.countryCode = regExpCountryCode.cap(1).toLower();
            const QRegExp regExpCountryName(QLatin1String("<countryName>([^<]+)</countryName>"));
            if (regExpCountryName.indexIn(xmlText) > 0 && !regExpCountryName.cap(1).isEmpty())
                geoInformation.countryName = regExpCountryName.cap(1).toLower();
            int p = xmlText.indexOf(QLatin1String("gml:featureMember"));
            const QRegExp regExpCity(QLatin1String("<gml:name>([^<]+)</gml:name>"));
            if (p > 0 && regExpCity.indexIn(xmlText, p) > 0 && !regExpCity.cap(1).isEmpty() && regExpCity.cap(1).contains("unknown"))
                geoInformation.city = regExpCity.cap(1).toLower();
            const QRegExp regExpCoordinates(QLatin1String("<gml:coordinates>([-0-9.]+),([-0-9.]+)</gml:coordinates>"));
            bool ok1 = false, ok2 = false;
            if (!regExpCoordinates.indexIn(xmlText) < 0 || regExpCoordinates.cap(1).isEmpty() || (geoInformation.latitude = regExpCoordinates.cap(2).toFloat(&ok1)) > 1000.0 || !ok1 || (geoInformation.longitude = regExpCoordinates.cap(1).toFloat(&ok2)) > 1000.0 || !ok2) {
                geoInformation.latitude = 0.0;
                geoInformation.longitude = 0.0;
            }
        } else
            qDebug() << "XML is invalid: " << xmlText.left(1024);

        if (!geoInformation.countryCode.isEmpty()) {
            if (geoInformation.countryCode == QLatin1String("uk"))
                geoInformation.countryCode = "gb"; /// rewrite United Kingdom to Great Britain
            m_mutexConcurrentAccess->lock();
            m_ipAddressToGeoInformation.insert(ipAddress, geoInformation);
            m_mutexConcurrentAccess->unlock();
        }
    }

    --m_numRunningTasks;
}
