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

#include <QString>
#include <QHash>
#include <QRegExp>

#include "guessing.h"
#include "general.h"

Guessing::Guessing()
{
    /// nothing
}

QString Guessing::fontToXML(const QString &fontName, const QString &typeName)
{
    QHash<QString, QString> name, beautifiedName, license, technology;
    name[""] = fontName;

    if (fontName.contains("Libertine")) {
        license["type"] = "open";
        license["name"] = "SIL Open Font License;GNU General Public License";
    } else if (fontName.contains("Nimbus")) {
        license["type"] = "open";
    } else if (fontName.contains("Liberation")) {
        license["type"] = "open";
    } else if (fontName.contains("DejaVu")) {
        license["type"] = "open";
    } else if (fontName.contains("Ubuntu")) {
        license["type"] = "open";
        license["name"] = "Ubuntu Font Licence";
    } else if (fontName.contains("Gentium")) {
        license["type"] = "open";
    } else if (fontName.startsWith("FreeSans") || fontName.startsWith("FreeSerif") || fontName.startsWith("FreeMono")) {
        license["type"] = "open";
    } else if (fontName.contains("Vera") || fontName.contains("Bera")) {
        license["type"] = "open";
    } else if (fontName.contains("Computer Modern") || fontName.startsWith("CM")) {
        license["type"] = "open";
        license["name"] = "SIL Open Font License";
    } else if (fontName.contains("Marvosym")) {
        license["type"] = "open";
        license["name"] = "SIL Open Font License";
    } else if (fontName.contains("OpenSymbol")) {
        license["type"] = "open";
        license["name"] = "LGPLv3?";
    } else if (fontName.startsWith("MnSymbol")) {
        license["type"] = "open";
        license["name"] = "PD";
    } else if (fontName.startsWith("Gotham") || fontName.startsWith("NewLibrisSerif")) {
        license["type"] = "proprietary"; // fonts used by Forsakringskassan
    } else if (fontName.startsWith("Zapf") || fontName.startsWith("Frutiger")) {
        license["type"] = "proprietary";
    } else if (fontName.startsWith("Arial") || fontName.startsWith("Verdana") || fontName.startsWith("TimesNewRoman") || fontName.startsWith("CourierNew") || fontName.startsWith("Georgia") || fontName == QLatin1String("Symbol")) {
        license["type"] = "proprietary"; // Microsoft
    } else if (fontName.startsWith("Lucinda") || fontName.startsWith("Trebuchet") || fontName.startsWith("Franklin Gothic") || fontName.startsWith("Century Schoolbook") || fontName.startsWith("CenturySchoolbook")) {
        license["type"] = "proprietary"; // Microsoft
    } else if (fontName.startsWith("Calibri") || fontName.startsWith("Cambria")  || fontName.startsWith("Constantia") || fontName.startsWith("Candara") || fontName.startsWith("Corbel") || fontName.startsWith("Consolas")) {
        license["type"] = "proprietary"; // Microsoft ClearType Font Collection
    } else if (fontName.contains("Univers")) {
        license["type"] = "proprietary"; // Linotype
    } else if (fontName.contains("Helvetica") && fontName.contains("Neue")) {
        license["type"] = "proprietary"; // Neue Helvetica by Linotype
    } else if (fontName.startsWith("Times") || fontName.startsWith("Tahoma") ||  fontName.startsWith("Courier") || fontName.contains("Helvetica") || fontName.contains("Wingdings")) {
        license["type"] = "proprietary";
    } else if (fontName.startsWith("SymbolMT")) {
        license["type"] = "proprietary"; // MonoType's font as shipped with Windows
    } else if (fontName.startsWith("Bembo") || fontName.startsWith("Rockwell") || fontName.startsWith("Perpetua")) {
        license["type"] = "proprietary"; // MonoType
    } else if (fontName.startsWith("ACaslon") || fontName.contains("EuroSans") || fontName.startsWith("MinionPro")) {
        license["type"] = "proprietary"; // Adobe
    } else if (fontName.contains("Officina")) {
        license["type"] = "proprietary"; // ITC
    } else if (fontName.startsWith("Bookman Old Style") || fontName.startsWith("Gill Sans")) {
        license["type"] = "proprietary"; // multiple alternatives
    } else
        license["type"] = "unknown";

    /// rumor: "MT" stands for MonoType's variant of a font
    QString bName = fontName;
    bName = bName.replace(QRegExp(QLatin1String("((Arial|Times|Courier)\\S*)(PS)?MT")), QLatin1String("\\1"));
    bool keepRomanAsSuffix = bName.startsWith(QLatin1String("TimesNewRoman")) || bName.startsWith(QLatin1String("Times New Roman"));
    bName = bName.replace(QRegExp(QLatin1String("(PS|FK)?[_-,.+]?(Semi(bold(It)?)?|Medium(It(alic)?|Oblique)?|Bold(It(alic)?|Oblique)?|Ital(ic)?|Light(It(alic)?|Oblique)?|Heavy(It(alic)?|Oblique)?|Roman|Upright|Regu(lar)?(It(alic)?|Oblique)?|Book(It(alic)?|Oblique)?|SC)(H|MT|[T]?OsF|PS)?")), QLatin1String(""));
    if (keepRomanAsSuffix) bName = bName.replace(QLatin1String("TimesNew"), QLatin1String("TimesNewRoman")).replace(QLatin1String("Times New"), QLatin1String("Times New Roman"));
    beautifiedName[""] = DocScan::xmlify(bName);

    QString text = typeName.toLower();
    if (text.indexOf("truetype") >= 0)
        technology["type"] = "truetype";
    else if (text.indexOf("type1") >= 0)
        technology["type"] = "type1";
    else if (text.indexOf("type3") >= 0)
        technology["type"] = "type3";

    const QString result = DocScan::formatMap("name", name) + DocScan::formatMap("beautified", beautifiedName) + DocScan::formatMap("technology", technology) + DocScan::formatMap("license", license);

    return result;
}
