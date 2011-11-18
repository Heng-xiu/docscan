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

#ifndef GENERAL_H
#define GENERAL_H

#include <QString>
#include <QDate>

namespace DocScan
{
/**
 * Make a text XML-safe by rewriting critical symbols such as <, & or, >.
 *
 * @param text plain text
 * @return text encoded as XML
 */
QString xmlify(QString text);

/**
 * Rewrite XML encodings like &aml; back to plain text like &.
 *
 * @param xml text encoded as XML
 * @return decoded plain text
 */
QString dexmlify(QString xml);

/**
 * Create an XML representation like
 * '<date epoch="1317333600" base="creation" year="2011" month="9" day="30">2011-09-30</date>'
 * for a given date.
 *
 * @param date date to encode
 * @param base value for the 'base' attribte
 * @return XML representation
 */
QString formatDate(const QDate &date, const QString &base);
}

#endif // GENERAL_H
