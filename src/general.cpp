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

#include <QRegExp>

#include "general.h"

namespace DocScan
{
QString xmlify(QString text)
{
    return text.replace(QRegExp("[^'a-z0-9,;.:-_+\\}{@|* !\"#%&/()=?åäöü]", Qt::CaseInsensitive), "").replace(QChar('&'), "&amp;").replace(QChar('<'), "&lt;").replace(QChar('>'), "&gt;").replace(QChar('"'), "'").simplified();
}

QString formatDate(const QDate &date, const QString &base)
{
    return QString("<date epoch=\"%6\" base=\"%5\" year=\"%1\" month=\"%2\" day=\"%3\">%4</date>\n").arg(date.year()).arg(date.month()).arg(date.day()).arg(date.toString(Qt::ISODate)).arg(base).arg(QString::number(QDateTime(date).toTime_t()));
}
}
