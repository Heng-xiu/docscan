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

#ifndef GEOIP_H
#define GEOIP_H

#include <QObject>
#include <QSet>
#include <QMap>
#include <QHostInfo>

#include "watchable.h"

class QMutex;
class QNetworkAccessManager;

/**
 * Retrieve geographical information of hosts by using geolocation services.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class GeoIP : public QObject, public Watchable
{
    Q_OBJECT
public:
    typedef struct {
        QString countryCode;
        QString countryName;
        QString city;
        float latitude, longitude;
    } GeoInformation;

    /**
     * Create instance and use the supplied network access manager for network connections.
     */
    explicit GeoIP(QNetworkAccessManager *networkAccessManager, QObject *parent = 0);
    ~GeoIP();

    /**
     * Start a search for the specified host. Search is asynchronous and will set this object alive.
     * Retrieve the found information later via getGeoInformation.
     * @see getGeoInformation(const QString &hostName) const
     *
     * @param hostName host's name
     */
    void lookupHost(const QString &hostName);

    /**
     * Retrieve geographical information about host name. This requires that
     * lookupHost has been issued before for this host. This function is blocking.
     * @see lookupHost(const QString &hostName)
     *
     * @param hostName host's name
     * @return geographical information for this host if known, otherwise an empty GeoInformation object
     */
    GeoInformation getGeoInformation(const QString &hostName) const;

    bool isAlive();

private:
    QNetworkAccessManager *m_networkAccessManager;

    QMutex *m_mutexConcurrentAccess;
    int m_numRunningTasks;

    QSet<QString> m_knownHostnames;
    QMap<QString, QString> m_hostnameToIPaddress;
    QMap<QString, GeoInformation> m_ipAddressToGeoInformation;

private slots:
    void gotHostInfo(const QHostInfo &hostInfo);
    void finishedGeoInfo();

};

#endif // GEOIP_H
