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

QString xmlify(const QString &text)
{
    QString result = text;
    result = result.replace(QRegExp(QLatin1String("[^-^[]'a-z0-9,;.:_+\\}{@|* !\"#%&/()=?åäöü]"), Qt::CaseInsensitive), QLatin1String(""));

    /// remove unprintable or control characters
    for (int i = 0; i < result.length(); ++i)
        if (result[i].unicode() < 0x0020 || result[i].unicode() >= 0x0100 || !result[i].isPrint()) {
            result = result.left(i) + result.mid(i + 1);
            --i;
        }

    result = result.replace(QChar('&'), QLatin1String("&amp;"));
    result = result.replace(QChar('<'), QLatin1String("&lt;")).replace(QChar('>'), QLatin1String("&gt;"));
    result = result.replace(QChar('"'), QLatin1String("&quot;")).replace(QChar('\''), QLatin1String("&apos;"));
    result = result.simplified();
    return result;
}

QString dexmlify(const QString &xml)
{
    QString result = xml;
    result = result.replace(QLatin1String("&lt;"), QChar('<')).replace(QLatin1String("&gt;"), QChar('>'));
    result = result.replace(QLatin1String("&quot;"), QChar('"'));
    result = result.replace(QLatin1String("&apos;"), QChar('\''));
    result = result.replace(QLatin1String("&amp;"), QChar('&'));
    return result;
}

QString formatDate(const QDate &date, const QString &base)
{
    return QString("<date base=\"%5\" day=\"%3\" epoch=\"%6\" month=\"%2\" year=\"%1\">%4</date>\n").arg(date.year()).arg(date.month()).arg(date.day()).arg(date.toString(Qt::ISODate)).arg(base).arg(QString::number(QDateTime(date).toTime_t()));
}

QString formatMap(const QString &key, const QHash<QString, QString> &attrs)
{
    if (attrs.isEmpty()) return QLatin1String("");

    const QString body = DocScan::xmlify(attrs[""]);
    QString result = QString("<%1").arg(key);
    for (QHash<QString, QString>::ConstIterator it = attrs.constBegin(); it != attrs.constEnd(); ++it)
        if (!it.key().isEmpty())
            result.append(QString(" %1=\"%2\"").arg(it.key()).arg(DocScan::xmlify(it.value())));

    if (body.isEmpty())
        result.append(" />\n");
    else
        result.append(">").append(body).append(QString("</%1>\n").arg(key));

    return result;
}

} // end of namespace
