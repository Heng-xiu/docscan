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
#include <QTextStream>

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
        qint64 bytesWritten = aspell.write(text.toUtf8());
        if (bytesWritten < 0) return QStringList(); /// something went wrong
        aspell.closeWriteChannel();

        QTextStream ts(&aspell);
        ts.setCodec("UTF-8");
        while (aspell.waitForReadyRead(10000)) {
            QString line;
            while (!(line = ts.readLine()).isNull()) {
                wordList << line;
            }
        }
        if (!aspell.waitForFinished(10000))
            aspell.kill();
    }
    return wordList;
}

QString FileAnalyzerAbstract::guessLanguage(const QString &text) const
{
    int count = std::numeric_limits<int>::max();
    QString best = QString::null;

    foreach(QString lang, getAspellLanguages()) {
        int c = runAspell(text, lang).count();
        if (c > 0 && c < count) { /// if c==0, no misspelled words where found, likely due to an error
            count = c;
            best = lang;
        }
    }

    return best;
}

QStringList FileAnalyzerAbstract::getAspellLanguages() const
{
    if (aspellLanguages.isEmpty()) {
        QRegExp language(QLatin1String("^[a-z]{2}(_[A-Z]{2})?$"));
        QProcess aspell(qApp);
        QStringList args = QStringList() << "dicts";
        aspell.start("/usr/bin/aspell", args);
        if (aspell.waitForStarted(10000)) {
            aspell.closeWriteChannel();
            while (aspell.waitForReadyRead(10000)) {
                while (aspell.canReadLine()) {
                    const QString line = aspell.readLine().simplified();
                    if (language.indexIn(line) >= 0) {
                        aspellLanguages << language.cap(0);
                    }
                }
            }
            if (!aspell.waitForFinished(10000))
                aspell.kill();
        }

        //emit analysisReport(QString("<initialization source=\"aspell\" type=\"languages\">%1</initialization>\n").arg(aspellLanguages.join(",")));
    }

    return aspellLanguages;
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
    } else if (text.indexOf("pdftex") >= 0) {
        static const QRegExp pdftexVersion("\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "pdftex ";
        result["product"] = "pdftex";
        if (pdftexVersion.indexIn(text) >= 0)
            result["version"] = pdftexVersion.cap(0);
    } else if (text.indexOf("latex") >= 0) {
        result["manufacturer"] = "latex ";
        result["product"] = "latex";
    } else if (text.indexOf("dvipdfm") >= 0) {
        static const QRegExp dvipdfmVersion("\\b\\d+(\\.\\d+)+[a-z]*\\b");
        result["manufacturer"] = "dvipdfm ";
        result["product"] = "dvipdfm";
        if (dvipdfmVersion.indexIn(text) >= 0)
            result["version"] = dvipdfmVersion.cap(0);
    } else if (text.indexOf("tex output") >= 0) {
        static const QRegExp texVersion("\\b\\d+([.:]\\d+)+\\b");
        result["manufacturer"] = "tex ";
        result["product"] = "tex";
        if (texVersion.indexIn(text) >= 0)
            result["version"] = texVersion.cap(0);
    } else if (text.indexOf("koffice") >= 0) {
        static const QRegExp kofficeVersion("/(d+([.]\\d+)*)\\b");
        result["manufacturer"] = "kde";
        result["product"] = "koffice";
        if (kofficeVersion.indexIn(text) >= 0)
            result["version"] = kofficeVersion.cap(1);
    } else if (text.indexOf("calligra") >= 0) {
        static const QRegExp calligraVersion("/(d+([.]\\d+)*)\\b");
        result["manufacturer"] = "kde";
        result["product"] = "calligra";
        if (calligraVersion.indexIn(text) >= 0)
            result["version"] = calligraVersion.cap(1);
    } else if (text.indexOf("abiword") >= 0) {
        result["manufacturer"] = "abisource";
        result["product"] = "abiword";
    } else if (text.indexOf("office_one") >= 0) {
        checkOOoVersion = true;
        result["product"] = "office_one";
        result["based-on"] = "openoffice";
    } else if (text.indexOf("infraoffice") >= 0) {
        checkOOoVersion = true;
        result["product"] = "infraoffice";
        result["based-on"] = "openoffice";
    } else if (text.indexOf("aksharnaveen") >= 0) {
        checkOOoVersion = true;
        result["product"] = "aksharnaveen";
        result["based-on"] = "openoffice";
    } else if (text.indexOf("redoffice") >= 0) {
        checkOOoVersion = true;
        result["manufacturer"] = "china";
        result["product"] = "redoffice";
        result["based-on"] = "openoffice";
    } else if (text.indexOf("sun_odf_plugin") >= 0) {
        checkOOoVersion = true;
        result["manufacturer"] = "oracle";
        result["product"] = "odfplugin";
        result["based-on"] = "openoffice";
    } else if (text.indexOf("libreoffice") >= 0) {
        checkOOoVersion = true;
        result["manufacturer"] = "tdf";
        result["product"] = "libreoffice";
        result["based-on"] = "openoffice";
    }  else if (text.indexOf("lotus symphony") >= 0) {
        static const QRegExp lotusSymphonyVersion("Symphony (\\d+(\\.\\d+)*)");
        result["manufacturer"] = "ibm";
        result["product"] = "lotus-symphony";
        result["based-on"] = "openoffice";
        if (lotusSymphonyVersion.indexIn(text) >= 0)
            result["version"] = lotusSymphonyVersion.cap(1);
    }  else if (text.indexOf("Lotus_Symphony") >= 0) {
        checkOOoVersion = true;
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
            result["manufacturer"] = "planamesa";
            result["product"] = "neooffice";
            result["based-on"] = "openoffice";
        } else {
            result["manufacturer"] = "oracle";
            result["product"] = "openoffice";
        }
    } else if (text == QLatin1String("writer") || text == QLatin1String("calc") || text == QLatin1String("impress")) {
        /// for Creator/Editor string
        result["manufacturer"] = "oracle;tdf";
        result["product"] = "openoffice;libreoffice";
        result["based-on"] = "openoffice";
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
    } else if (text.indexOf("pdfwriter") >= 0) {
        static const QRegExp pdfwriterVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "pdfwriter";
        if (pdfwriterVersion.indexIn(text) >= 0)
            result["version"] = pdfwriterVersion.cap(0);
    } else if (text.indexOf("easypdf") >= 0) {
        static const QRegExp easypdfVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "bcl";
        result["product"] = "easypdf";
        if (easypdfVersion.indexIn(text) >= 0)
            result["version"] = easypdfVersion.cap(0);
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
    } else if (text.indexOf("pagemaker") >= 0) {
        static const QRegExp pagemakerVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "pagemaker";
        if (pagemakerVersion.indexIn(text) >= 0)
            result["version"] = pagemakerVersion.cap(0);
    } else if (text.indexOf("acrobat capture") >= 0) {
        static const QRegExp acrobatCaptureVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "acrobatcapture";
        if (acrobatCaptureVersion.indexIn(text) >= 0)
            result["version"] = acrobatCaptureVersion.cap(0);
    } else if (text.indexOf("acrobat pro") >= 0) {
        static const QRegExp acrobatProVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "acrobatpro";
        if (acrobatProVersion.indexIn(text) >= 0)
            result["version"] = acrobatProVersion.cap(0);
    } else if (text.indexOf("acrobat") >= 0) {
        static const QRegExp acrobatVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        result["product"] = "acrobat";
        if (acrobatVersion.indexIn(text) >= 0)
            result["version"] = acrobatVersion.cap(0);
    } else if (text.indexOf("livecycle") >= 0) {
        static const QRegExp livecycleVersion("\\b\\d+(\\.\\d+)+[a-z]?\\b");
        result["manufacturer"] = "adobe";
        int regExpPos;
        if ((regExpPos = livecycleVersion.indexIn(text)) >= 0)
            result["version"] = livecycleVersion.cap(0);
        if (regExpPos <= 0)
            regExpPos = 1024;
        QString product = text;
        result["product"] = product.left(regExpPos - 1).replace("adobe", "").replace(livecycleVersion.cap(0), "").replace(" ", "");
    } else if (text.indexOf("photoshop") >= 0) {
        result["manufacturer"] = "adobe";
        result["product"] = "photoshop";
    } else if (text.indexOf("adobe") >= 0) {
        /// some unknown Adobe product
        static const QRegExp adobeVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "adobe";
        if (adobeVersion.indexIn(text) >= 0)
            result["version"] = adobeVersion.cap(0);
        QString product = text;
        result["product"] = product.replace("adobe", "").replace(adobeVersion.cap(0), "").replace(" ", "");
    } else if (text.contains("pages")) {
        result["manufacturer"] = "apple";
        result["product"] = "pages";
    } else if (text.contains("keynote")) {
        static const QRegExp keynoteVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "apple";
        result["product"] = "keynote";
        if (keynoteVersion.indexIn(text) >= 0)
            result["version"] = keynoteVersion.cap(0);
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
        result["opsys"] = QLatin1String("windows");
        if (pscriptVersion.indexIn(text) >= 0)
            result["version"] = pscriptVersion.cap(0);
    } else if (text.indexOf("quarkxpress") >= 0) {
        static const QRegExp quarkxpressVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "quark";
        result["product"] = "xpress";
        if (quarkxpressVersion.indexIn(text) >= 0)
            result["version"] = quarkxpressVersion.cap(0);
    } else if (text.indexOf("pdfcreator") >= 0) {
        static const QRegExp pdfcreatorVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "pdfforge";
        result["product"] = "pdfcreator";
        result["opsys"] = QLatin1String("windows");
        if (pdfcreatorVersion.indexIn(text) >= 0)
            result["version"] = pdfcreatorVersion.cap(0);
    } else if (text.indexOf("pdf printer") >= 0) {
        result["manufacturer"] = "bullzip";
        result["product"] = "pdfprinter";
    } else if (text.contains("aspose") && text.contains("words")) {
        static const QRegExp asposewordsVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "aspose";
        result["product"] = "aspose.words";
        if (asposewordsVersion.indexIn(text) >= 0)
            result["version"] = asposewordsVersion.cap(0);
    } else if (text.contains("arcmap")) {
        static const QRegExp arcmapVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "esri";
        result["product"] = "arcmap";
        if (arcmapVersion.indexIn(text) >= 0)
            result["version"] = arcmapVersion.cap(0);
    } else if (text.contains("ocad")) {
        static const QRegExp ocadVersion("\\b\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "ocad";
        result["product"] = "ocad";
        if (ocadVersion.indexIn(text) >= 0)
            result["version"] = ocadVersion.cap(0);
    } else if (text.contains("gnostice")) {
        static const QRegExp gnosticeVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "gnostice";
        if (gnosticeVersion.indexIn(text) >= 0)
            result["version"] = gnosticeVersion.cap(0);
        QString product = text;
        result["product"] = product.replace("gnostice", "").replace(gnosticeVersion.cap(0), "").replace(" ", "");
    } else if (text.contains("canon")) {
        static const QRegExp canonVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "canon";
        if (canonVersion.indexIn(text) >= 0)
            result["version"] = canonVersion.cap(0);
        QString product = text;
        result["product"] = product.replace("canon", "").replace(canonVersion.cap(0), "").replace(" ", "");
    } else if (text.contains("toshiba") || text.contains("mfpimglib")) {
        static const QRegExp toshibaVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "toshiba";
        if (toshibaVersion.indexIn(text) >= 0)
            result["version"] = toshibaVersion.cap(0);
        QString product = text;
        result["product"] = product.replace("toshiba", "").replace(toshibaVersion.cap(0), "").replace(" ", "");
    } else if (text.contains("konica") || text.contains("minolta")) {
        static const QRegExp konicaMinoltaVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "konica;minolta";
        if (konicaMinoltaVersion.indexIn(text) >= 0)
            result["version"] = konicaMinoltaVersion.cap(0);
        QString product = text;
        result["product"] = product.replace("konica", "").replace("minolta", "").replace(konicaMinoltaVersion.cap(0), "").replace(" ", "");
    } else if (text.contains("corel")) {
        static const QRegExp corelVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        result["manufacturer"] = "corel";
        if (corelVersion.indexIn(text) >= 0)
            result["version"] = corelVersion.cap(0);
        QString product = text;
        result["product"] = product.replace("corel", "").replace(corelVersion.cap(0), "").replace(" ", "");
    } else if (text.contains("scansoft pdf create")) {
        static const QRegExp scansoftVersion("\\b([a-zA-Z]+[ ])?[A-Za-z0-9]+\\b");
        result["manufacturer"] = "scansoft";
        result["product"] = "pdfcreate";
        if (scansoftVersion.indexIn(text) >= 0)
            result["version"] = scansoftVersion.cap(0);
    } else if (text.contains("alivepdf")) {
        static const QRegExp alivepdfVersion("\\b\\d+(\\.\\d+)+( RC)?\\b");
        result["manufacturer"] = "thibault.imbert";
        result["product"] = "alivepdf";
        if (alivepdfVersion.indexIn(text) >= 0)
            result["version"] = alivepdfVersion.cap(0);
        result["opsys"] = QLatin1String("flash");
    } else if (text == QLatin1String("google")) {
        result["manufacturer"] = "google";
        result["product"] = "docs";
    } else if (!text.contains("words")) {
        static const QRegExp microsoftProducts("powerpoint|excel|word|outlook");
        static const QRegExp microsoftVersion("\\b(starter )?(20[01][0-9]|1?[0-9]\\.[0-9]+|9[5-9])\\b");
        if (microsoftProducts.indexIn(text) >= 0) {
            result["manufacturer"] = "microsoft";
            result["product"] = microsoftProducts.cap(0);
            if (!result.contains("version") && microsoftVersion.indexIn(text) >= 0)
                result["version"] = microsoftVersion.cap(2);
            if (!result.contains("subversion") && !microsoftVersion.cap(1).isEmpty())
                result["subversion"] = microsoftVersion.cap(1);

            if (text.contains(QLatin1String("Macintosh")) || text.contains(QLatin1String("Mac OS X")))
                result["opsys"] = QLatin1String("macosx");
            else
                result["opsys"] = QLatin1String("windows?");
        }
    }

    if (checkOOoVersion) {
        static const QRegExp OOoVersion("[a-z]/(\\d(\\.\\d+)+)(_Beta|pre)?[$a-z]", Qt::CaseInsensitive);
        if (OOoVersion.indexIn(text) >= 0)
            result["version"] = OOoVersion.cap(1);

        if (text.indexOf(QLatin1String("unix")) >= 0)
            result["opsys"] = QLatin1String("generic-unix");
        else if (text.indexOf(QLatin1String("linux")) >= 0)
            result["opsys"] = QLatin1String("linux");
        else if (text.indexOf(QLatin1String("win32")) >= 0)
            result["opsys"] = QLatin1String("windows");
        else if (text.indexOf(QLatin1String("solaris")) >= 0)
            result["opsys"] = QLatin1String("solaris");
        else if (text.indexOf(QLatin1String("freebsd")) >= 0)
            result["opsys"] = QLatin1String("bsd");
    }

    if (!result.contains("opsys")) {
        if (text.contains(QLatin1String("Macint")))
            result["opsys"] = QLatin1String("macosx");
        else if (text.contains(QLatin1String("Windows")))
            result["opsys"] = QLatin1String("windows");
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
        result += formatMap("name", programMap);
    }

    return result;
}

QString FileAnalyzerAbstract::guessFont(const QString &fontName, const QString &typeName) const
{
    QMap<QString, QString> name, beautifiedName, license, technology;
    name[""] = DocScan::xmlify(fontName);

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
    } else if (fontName.startsWith("Gotham") || fontName.startsWith("NewLibrisSerif")) {
        license["type"] = "proprietary"; // fonts used by Forsakringskassan
    } else if (fontName.startsWith("Zapf") || fontName.startsWith("Frutiger")) {
        license["type"] = "proprietary";
    } else if (fontName.startsWith("Arial") || fontName.startsWith("Verdana") || fontName.startsWith("TimesNewRoman") || fontName.startsWith("CourierNew")) {
        license["type"] = "proprietary"; // Microsoft
    } else if (fontName.startsWith("Calibri") || fontName.startsWith("Cambria")  || fontName.startsWith("Constantia") || fontName.startsWith("Candara") || fontName.startsWith("Corbel") || fontName.startsWith("Consolas")) {
        license["type"] = "proprietary"; // Microsoft ClearType Font Collection
    } else if (fontName.contains("Helvetica") && fontName.contains("Neue")) {
        license["type"] = "proprietary"; // Neue Helvetica by Linotype
    } else if (fontName.startsWith("Times") || fontName.startsWith("Courier") || fontName.contains("Helvetica")) {
        license["type"] = "proprietary";
    } else if (fontName.startsWith("SymbolMT")) {
        license["type"] = "proprietary"; // MonoType's font as shipped with Windows
    } else if (fontName.startsWith("Bembo") || fontName.startsWith("Rockwell")) {
        license["type"] = "proprietary"; // MonoType
    } else if (fontName.startsWith("ACaslon")) {
        license["type"] = "proprietary"; // Adobe
    } else
        license["type"] = "unknown";

    /// rumor: "MT" stands for MonoType's variant of a font
    QString bName = fontName;
    bName = bName.replace(QRegExp(QLatin1String("((Arial|Times|Courier)\\S*)(PS)?MT")), QLatin1String("\\1"));
    bool keepRomanAsSuffix = bName.startsWith(QLatin1String("TimesNewRoman"));
    bName = bName.replace(QRegExp(QLatin1String("(PS|FK)?[_-,.+]?(Semibold(It)?|Medium(It(alic)?|Oblique)?|Bold(It(alic)?|Oblique)?|Ital(ic)?|Light(It(alic)?|Oblique)?|Heavy(It(alic)?|Oblique)?|Roman|Upright|Regu(lar)?(It(alic)?|Oblique)?|Book(It(alic)?|Oblique)?|SC)(H|MT|[T]?OsF)?")), QLatin1String(""));
    if (keepRomanAsSuffix) bName = bName.replace(QLatin1String("TimesNew"), QLatin1String("TimesNewRoman"));
    beautifiedName[""] = DocScan::xmlify(bName);

    QString text = typeName.toLower();
    if (text.indexOf("truetype") >= 0)
        technology["type"] = "truetype";
    else if (text.indexOf("type1") >= 0)
        technology["type"] = "type1";
    else if (text.indexOf("type3") >= 0)
        technology["type"] = "type3";

    return formatMap("name", name) + formatMap("beautified", beautifiedName) + formatMap("technology", technology) + formatMap("license", license);
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
    else if (mmh >= 214 && mmh <= 218 && mmw >= 277 && mmw <= 281)
        formatName = "Letter";
    else if (mmw >= 214 && mmw <= 218 && mmh >= 254 && mmh <= 258)
        formatName = "Legal";
    else if (mmh >= 214 && mmh <= 218 && mmw >= 254 && mmw <= 258)
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
