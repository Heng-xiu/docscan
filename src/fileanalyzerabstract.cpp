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

#include <limits>

#include <QProcess>
#include <QCoreApplication>
#include <QDate>

#include "fileanalyzerabstract.h"
#include "general.h"

FileAnalyzerAbstract::FileAnalyzerAbstract(QObject *parent)
    : QObject(parent)
{
}

QStringList FileAnalyzerAbstract::runAspell(const QString &text, const QString &dictionary) const
{
    QStringList wordList;
    QProcess aspell(QCoreApplication::instance());
    QStringList args = QStringList() << "-d" << dictionary << "list";
    aspell.start("/usr/bin/aspell", args);
    if (aspell.waitForStarted(10000)) {
        aspell.write(text.toUtf8());
        aspell.closeWriteChannel();
        while (aspell.waitForReadyRead(10000)) {
            while (aspell.canReadLine()) {
                wordList << aspell.readLine();
            }
        }
        if (!aspell.waitForFinished(10000))
            aspell.kill();
    }
    return wordList;
}

QString FileAnalyzerAbstract::guessLanguage(const QString &text) const
{
    Q_UNUSED(text);
    //int count = std::numeric_limits<int>::max();
    QString best = QString::null;

    /*
    foreach(QString lang, getAspellLanguages()) {
        int c = runAspell(text, lang).count();
        if (c < count) {
            count = c;
            best = lang;
        }
    }
    */

    return best;
}

QStringList FileAnalyzerAbstract::getAspellLanguages() const
{
    if (aspellLanguages.isEmpty()) {
        /*
        QRegExp language("^[ ]+([a-z]{2})( |$)");
        QProcess aspell(this);
        QStringList args = QStringList() << "--help";
        aspell.start("/usr/bin/aspell", args);
        if (aspell.waitForStarted(10000)) {
            aspell.closeWriteChannel();
            while (aspell.waitForReadyRead(10000)) {
                while (aspell.canReadLine()) {
                    if (language.indexIn(aspell.readLine()) >= 0) {
                        aspellLanguages << language.cap(1);
                    }
                }
            }
            if (!aspell.waitForFinished(10000))
                aspell.kill();
        }
        */
        aspellLanguages << "de" << "en" << "sv" << "fi" << "fr";
    }
    return aspellLanguages;
}

QMap<QString, QString> FileAnalyzerAbstract::guessLicense(const QString &license) const
{
    const QString text = license.toLower();
    QMap<QString, QString> result;

    if (text.contains("primopdf") || text.contains("freeware")) {
        result["type"] = "beer";
    } else if (text.contains("microsoft") || text.contains("framemaker") || text.contains("adobe") || text.contains("acrobat") || text.contains("excel") || text.contains("powerpoint") || text.contains("quartz") || text.contains("pdfxchange") || text.contains("freehand") || text.contains("quarkxpress") || text.contains("illustrator") || text.contains("hp pdf") || text.contains("pscript5") || text.contains("s.a.") || text.contains("KDK") || text.contains("scansoft") || text.contains("crystal")) {
        result["type"] = "proprietary";
    } else if (text.contains("neooffice") || text.contains("broffice") || text.contains("koffice") || text.contains("calligra")) {
        result["type"] = "open";
    } else if (text.contains("openoffice") || text.contains("libreoffice") || text == "writer") {
        result["type"] = "open";
    } else if (text.contains("pdftex") || text.contains("ghostscript") || text.contains("dvips") || text.contains("pdfcreator")) {
        result["type"] = "open";
    } else if (text.contains("gnu") || text.contains("gpl") || text.contains("bsd") || text.contains("apache")) {
        result["type"] = "open";
    }

    return result;
}

QMap<QString, QString> FileAnalyzerAbstract::guessOpSys(const QString &opsys) const
{
    const QString text = opsys.toLower();
    QMap<QString, QString> result;

    if (text.indexOf("neooffice") >= 0 || text.indexOf("quartz pdfcontext") >= 0 || text.indexOf("mac os x") >= 0 || text.indexOf("macintosh") >= 0) {
        result["type"] = "mac";
        result["license"] = "proprietary";
    } else if (text.indexOf("win32") >= 0) {
        result["type"] = "windows";
        result["arch"] = "win32";
        result["license"] = "proprietary";
    } else if (text.indexOf("win64") >= 0) {
        result["type"] = "windows";
        result["arch"] = "win64";
        result["license"] = "proprietary";
    } else if (text.indexOf("microsoftoffice") >= 0 || text.indexOf("windows") >= 0 || text.indexOf(".dll") >= 0 || text.indexOf("pdf complete") >= 0 || text.indexOf("nitro pdf") >= 0 || text.indexOf("primopdf") >= 0) {
        result["type"] = "windows";
        result["license"] = "proprietary";
    } else if (text.indexOf("linux") >= 0) {
        result["type"] = "linux";
        result["license"] = "open";
    } else if (text.indexOf("unix") >= 0) {
        result["type"] = "unix";
    } else if (text.indexOf("solaris") >= 0) {
        result["type"] = "unix";
        result["brand"] = "solaris";
    }

    return result;
}

QMap<QString, QString> FileAnalyzerAbstract::guessProgram(const QString &program) const
{
    const QString text = program.toLower();
    QMap<QString, QString> result;
    result[""] = DocScan::xmlify(program);
    bool checkOOoVersion = false;

    if (text.indexOf("dvips") >= 0) {
        static const QRegExp radicaleyeVersion("\\b\\d+\\.\\d+[a-z]*\\b");
        result["manufacturer"] = "radicaleye";
        if (radicaleyeVersion.indexIn(text) >= 0)
            result["version"] = radicaleyeVersion.cap(0);
    } else if (text.indexOf("ghostscript") >= 0) {
        static const QRegExp ghostscriptVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "artifex ";
        result["product"] = "ghostscript";
        if (ghostscriptVersion.indexIn(text) >= 0)
            result["version"] = ghostscriptVersion.cap(0);
    } else if (text.indexOf("koffice") >= 0) {
        result["manufacturer"] = "kde";
    } else if (text.indexOf("abiword") >= 0) {
        result["manufacturer"] = "abisource";
        result["product"] = "abiword";
    } else if (text.indexOf("libreoffice") >= 0) {
        checkOOoVersion = true;
        result["manufacturer"] = "tdf";
        result["product"] = "libreoffice";
        result["based-on"] = "openoffice";
    }  else if (text.indexOf("lotus symphony") >= 0) {
        result["manufacturer"] = "ibm";
        result["product"] = "lotus-symphony";
        result["based-on"] = "openoffice";
    } else if (text.indexOf("openoffice") >= 0) {
        checkOOoVersion = true;
        if (text.indexOf("staroffice") >= 0) {
            result["manufacturer"] = "oracle";
            result["based-on"] = "openoffice";
            result["product"] = "staroffice";
        } else if (text.indexOf("broffice") >= 0) {
            result["product"] = "broffice";
            result["based-on"] = "openoffice";
        } else if (text.indexOf("neooffice") >= 0) {
            result["product"] = "neooffice";
            result["based-on"] = "openoffice";
        } else {
            result["manufacturer"] = "oracle";
            result["product"] = "openoffice";
        }
    } else if (text.indexOf("framemaker") >= 0) {
        static const QRegExp framemakerVersion("\\b\\d+(\\.\\d+)+(\\b|\\.|p\\d+)");
        result["manufacturer"] = "adobe";
        result["product"] = "framemaker";
        if (framemakerVersion.indexIn(text) >= 0)
            result["version"] = framemakerVersion.cap(0);
    } else if (text.indexOf("distiller") >= 0) {
        static const QRegExp distillerVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "distiller";
        if (distillerVersion.indexIn(text) >= 0)
            result["version"] = distillerVersion.cap(0);
    } else if (text.indexOf("pdf library") >= 0) {
        static const QRegExp pdflibraryVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "pdflibrary";
        if (pdflibraryVersion.indexIn(text) >= 0)
            result["version"] = pdflibraryVersion.cap(0);
    } else if (text.indexOf("pdfmaker") >= 0) {
        static const QRegExp pdfmakerVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "pdfmaker";
        if (pdfmakerVersion.indexIn(text) >= 0)
            result["version"] = pdfmakerVersion.cap(0);
    } else if (text.indexOf("indesign") >= 0) {
        static const QRegExp indesignVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "indesign";
        if (indesignVersion.indexIn(text) >= 0)
            result["version"] = indesignVersion.cap(0);
    } else if (text.indexOf("quartz") >= 0) {
        static const QRegExp quartzVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "apple";
        result["product"] = "quartz";
        if (quartzVersion.indexIn(text) >= 0)
            result["version"] = quartzVersion.cap(0);
    } else if (text.indexOf("pscript5.dll") >= 0) {
        static const QRegExp pscriptVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "microsoft";
        result["product"] = "pscript5";
        if (pscriptVersion.indexIn(text) >= 0)
            result["version"] = pscriptVersion.cap(0);
    } else if (text.indexOf("quarkxpress") >= 0) {
        static const QRegExp quarkxpressVersion(" (\\d+(\\.\\d+)+)\\b");
        result["manufacturer"] = "quark";
        result["product"] = "xpress";
        if (quarkxpressVersion.indexIn(text) >= 0)
            result["version"] = quarkxpressVersion.cap(1);
    } else {
        static const QRegExp microsoftProducts("powerpoint|excel|word|outlook");
        static const QRegExp microsoftVersion("\\b(20[01][0-9]|1?[0-9]\\.[0-9]+|9[5-9])\\b");
        if (microsoftProducts.indexIn(text) >= 0) {
            result["manufacturer"] = "microsoft";
            result["product"] = microsoftProducts.cap(0);
            if (!result.contains("version") && microsoftVersion.indexIn(text) >= 0)
                result["version"] = microsoftVersion.cap(0);
        }
    }

    if (checkOOoVersion) {
        static const QRegExp OOoVersion("[a-z]/(\\d(\\.\\d+)+)[$a-z]", Qt::CaseInsensitive);
        if (OOoVersion.indexIn(text) >= 0)
            result["version"] = OOoVersion.cap(1);
    }

    return result;
}

QString FileAnalyzerAbstract::guessTool(const QString &toolString, const QString &altToolString) const
{
    QString result;
    QString text;

    if (microsoftToolRegExp.indexIn(altToolString) == 0)
        text = microsoftToolRegExp.cap(1);
    else if (!toolString.isEmpty())
        text = toolString;
    else if (!altToolString.isEmpty())
        text = altToolString;

    if (!text.isEmpty()) {
        QMap<QString, QString> programMap = guessProgram(text);
        QMap<QString, QString> opSysMap = guessOpSys(text);

        QString version;
        if (programMap["manufacturer"] == "microsoft" && !(version = programMap["version"]).isEmpty()) {
            if (text.contains("Macintosh") || version == "98" || version == "2001" || version == "2004" || version == "2008" || version == "2011")
                opSysMap["type"] = "mac";
            else if (version == "97" || version == "2000" || version == "9.0" || version == "2002" || version == "10.0" || version == "2003" || version == "11.0" || version == "2007" || version == "12.0" || version == "2010" || version == "14.0")
                opSysMap["type"] = "windows";
            else
                opSysMap["remark"] = "unknown version: " + version;
        }

        result += formatMap("name", programMap);
        result += formatMap("license", guessLicense(text));
        result += formatMap("opsys", opSysMap);
    }

    return result;
}

QString FileAnalyzerAbstract::guessFont(const QString &fontName, const QString &typeName) const
{
    QMap<QString, QString> name, license, technology;
    name[""] = DocScan::xmlify(fontName);

    if (fontName.contains("Libertine")) {
        license["type"] = "open";
    } else if (fontName.contains("Nimbus")) {
        license["type"] = "open";
    } else if (fontName.contains("Liberation")) {
        license["type"] = "open";
    } else if (fontName.contains("DejaVu")) {
        license["type"] = "open";
    } else if (fontName.contains("Ubuntu")) {
        license["type"] = "open";
    } else if (fontName.contains("Gentium")) {
        license["type"] = "open";
    } else if (fontName.contains("Vera") || fontName.contains("Bera")) {
        license["type"] = "open";
    } else if (fontName.contains("Computer Modern")) {
        license["type"] = "open";
    } else {
        license["type"] = "proprietary";
    }

    QString text = typeName.toLower();
    if (text.indexOf("truetype") >= 0)
        technology["type"] = "truetype";
    else if (text.indexOf("type1") >= 0)
        technology["type"] = "type1";
    else if (text.indexOf("type3") >= 0)
        technology["type"] = "type3";

    return formatMap("name", name) + formatMap("technology", technology) + formatMap("license", license);
}

QString FileAnalyzerAbstract::formatDate(const QDate &date, const QString &base) const
{
    return QString("<date epoch=\"%6\" %5 year=\"%1\" month=\"%2\" day=\"%3\">%4</date>\n").arg(date.year()).arg(date.month()).arg(date.day()).arg(date.toString(Qt::ISODate)).arg(base.isEmpty() ? QLatin1String("") : QString("base=\"%1\"").arg(base)).arg(QString::number(QDateTime(date).toTime_t()));
}

QString FileAnalyzerAbstract::evaluatePaperSize(int mmw, int mmh) const
{
    QString formatName;

    if (mmw >= 208 && mmw <= 212 && mmh >= 295 && mmh <= 299)
        formatName = "A4";
    else if (mmh >= 208 && mmh <= 212 && mmw >= 295 && mmw <= 299)
        formatName = "A4";
    else if (mmw >= 214 && mmw <= 218 && mmh >= 277 && mmh <= 281)
        formatName = "Letter";
    else if (mmw >= 214 && mmw <= 218 && mmh >= 254 && mmh <= 258)
        formatName = "Legal";

    return QString("<papersize height=\"%1\" width=\"%2\" orientation=\"%4\">%3</papersize>\n").arg(mmh).arg(mmw).arg(formatName).arg(mmw > mmh ? "landscape" : "portrait");
}

QString FileAnalyzerAbstract::formatMap(const QString &key, const QMap<QString, QString> &attrs) const
{
    if (attrs.isEmpty()) return QLatin1String("");

    const QString body = DocScan::xmlify(attrs[""]);
    QString result = QString("<%1").arg(key);
    for (QMap<QString, QString>::ConstIterator it = attrs.constBegin(); it != attrs.constEnd(); ++it)
        if (!it.key().isEmpty())
            result.append(QString(" %1=\"%2\"").arg(it.key()).arg(DocScan::xmlify(it.value())));

    if (body.isEmpty())
        result.append(" />\n");
    else
        result.append(">").append(body).append(QString("</%1>\n").arg(key));

    return result;
}

QStringList FileAnalyzerAbstract::aspellLanguages;

const QRegExp FileAnalyzerAbstract::microsoftToolRegExp("^(Microsoft\\s(.+\\S) [ -][ ]?(\\S.*)$");
const QString FileAnalyzerAbstract::creationDate = QLatin1String("creation");
const QString FileAnalyzerAbstract::modificationDate = QLatin1String("modification");
