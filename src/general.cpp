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
#include <QStringList>

#include "general.h"

namespace DocScan
{

QString xmlNodeToText(const XMLNode &node)
{
    QString result = QString(QChar('<')).append(node.name);
    QStringList attributes = node.attributes.keys();
    attributes.sort();
    for (QStringList::ConstIterator it = attributes.constBegin(); it != attributes.constEnd(); ++it)
        result.append(QString(QLatin1String(" %1=\"%2\"")).arg(*it).arg(xmlify(node.attributes[*it])));

    if (node.text.isEmpty())
        result.append(QLatin1String(" />\n"));
    else {
        result.append(QLatin1String(">\n"));
        result.append(node.text);
        result.append(QString(QLatin1String("</%1>\n")).arg(node.name));
    }

    return result;
}

QString xmlify(QString text)
{
    return text.replace(QRegExp(QLatin1String("[^-'a-z0-9,;.:_+\\}{@|* !\"#%&/()=?åäöü]"), Qt::CaseInsensitive), "").replace(QChar('&'), QLatin1String("&amp;")).replace(QChar('<'), QLatin1String("&lt;")).replace(QChar('>'), QLatin1String("&gt;")).replace(QChar('"'), QLatin1String("&quot;")).replace(QChar('\''), QLatin1String("&apos;")).simplified();
}

QString dexmlify(QString xml)
{
    return xml.replace(QLatin1String("&lt;"), QChar('<')).replace(QLatin1String("&gt;"), QChar('>')).replace(QLatin1String("&quot;"), QChar('"')).replace(QLatin1String("&apos;"), QChar('\'')).replace(QLatin1String("&amp;"), QChar('&'));
}

QString formatDate(const QDate &date, const QString &base)
{
    return QString("<date base=\"%5\" day=\"%3\" epoch=\"%6\" month=\"%2\" year=\"%1\">%4</date>\n").arg(date.year()).arg(date.month()).arg(date.day()).arg(date.toString(Qt::ISODate)).arg(base).arg(QString::number(QDateTime(date).toTime_t()));
}
}
