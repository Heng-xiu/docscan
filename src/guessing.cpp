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

#include "guessing.h"

#include <QStringList>
#include <QHash>
#include <QVector>
#include <QRegExp>

#include "general.h"

Guessing::Guessing()
{
    /// nothing
}

QString Guessing::fontToXML(const QString &fontName, const QString &typeName)
{
    QHash<QString, QString> name, beautifiedName, license, technology;
    name[QStringLiteral("")] = fontName;
    license[QStringLiteral("type")] = QStringLiteral("unknown"); ///< default: license type is unknown

    if (fontName.contains(QStringLiteral("Libertine"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("SIL Open Font License;GNU General Public License");
    } else if (fontName.contains(QStringLiteral("Nimbus"))) {
        license[QStringLiteral("type")] = QStringLiteral("open"); /// URW++, released as GPL and AFPL
        license[QStringLiteral("name")] = QStringLiteral("GNU General Public License;Aladdin Free Public License");
    } else if (fontName.startsWith(QStringLiteral("URWPalladio"))) {
        license[QStringLiteral("type")] = QStringLiteral("open"); /// URW++, released as GPL and AFPL
    } else if (fontName.startsWith(QStringLiteral("Kp-")) || fontName.contains(QStringLiteral("-Kp-"))) {
        license[QStringLiteral("type")] = QStringLiteral("open"); /// 'Johannes Kepler' font based on URW++'s Palladio, by Christophe Caignaert
    } else if (fontName.contains(QStringLiteral("Liberation"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("GNU General Public License v2 with Font Exception");
    } else if (fontName.contains(QStringLiteral("DejaVu"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
    } else if (fontName.startsWith(QStringLiteral("Ubuntu"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("Ubuntu Font Licence");
    } else if (fontName.startsWith(QStringLiteral("Junicode"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("SIL Open Font License");
    } else if (fontName.contains(QStringLiteral("Gentium"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
    } else if (fontName.startsWith(QStringLiteral("FreeSans")) || fontName.startsWith(QStringLiteral("FreeSerif")) || fontName.startsWith(QStringLiteral("FreeMono"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
    } else if (fontName.contains(QStringLiteral("Vera")) || fontName.contains(QStringLiteral("Bera"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
    } else if (fontName.startsWith(QStringLiteral("TeX-")) || fontName.startsWith(QStringLiteral("TeXPa")) || fontName.startsWith(QStringLiteral("PazoMath"))) { /// TeX fonts
        license[QStringLiteral("type")] = QStringLiteral("open"); /// most likely
    } else if (fontName.endsWith(QStringLiteral("circle10"))) { /// TeX fonts: http://tug.ctan.org/info/fontname/texfonts.map
        license[QStringLiteral("type")] = QStringLiteral("open"); /// most likely
    } else if (fontName.contains(QStringLiteral("Old Standard")) || fontName.contains(QStringLiteral("OldStandard"))) {
        /// https://fontlibrary.org/en/font/old-standard
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("SIL Open Font License");
    } else if (fontName.contains(QStringLiteral("Computer Modern"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("SIL Open Font License");
    } else if (fontName.startsWith(QStringLiteral("cmr")) || fontName.startsWith(QStringLiteral("cmsy")) || fontName.startsWith(QStringLiteral("stmary")) || fontName.startsWith(QStringLiteral("wasy")) || fontName.contains(QRegExp(QStringLiteral("(^|_)([Cc]mr|[Cc]mmi|[Cc]msy|EUSM|CM|SF|MS)[A-Z0-9]+$")))) { /// TeX fonts
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("SIL Open Font License");
    } else if (fontName.contains(QStringLiteral("Marvosym"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("GUST Font License (GFL);LaTeX Project Public License (LPPL)");
    } else if (fontName.startsWith(QStringLiteral("LMSans")) || fontName.startsWith(QStringLiteral("LMRoman")) || fontName.startsWith(QStringLiteral("LMSlanted")) || fontName.startsWith(QStringLiteral("LMTypewriter")) || fontName.startsWith(QStringLiteral("LMMath")) || fontName.startsWith(QStringLiteral("LMMono"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("SIL Open Font License");
    } else if (fontName.contains(QStringLiteral("OpenSymbol"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("LGPLv3?");
    } else if (fontName.startsWith(QStringLiteral("MnSymbol"))) {
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("PD");
    } else if (fontName.startsWith(QStringLiteral("Arimo")) || fontName.startsWith(QStringLiteral("Tinos"))) {
        /// https://en.wikipedia.org/wiki/Croscore_fonts
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("Apache 2.0");
    } else if (fontName.startsWith(QStringLiteral("DroidSans")) || fontName.startsWith(QStringLiteral("DroidSerif")) || fontName.startsWith(QStringLiteral("DroidMono"))) {
        /// https://en.wikipedia.org/wiki/Droid_fonts
        license[QStringLiteral("type")] = QStringLiteral("open");
        license[QStringLiteral("name")] = QStringLiteral("Apache 2.0");
    } else if (fontName.contains(QStringLiteral("Menlo"))) {
        license[QStringLiteral("type")] = QStringLiteral("open"); /// Apple's new font based on Bitstream's Vera
    } else if (fontName.startsWith(QStringLiteral("Apple"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Most likely some Apple font
    } else if (fontName.startsWith(QStringLiteral("Geneva"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Apple
    } else if (fontName.startsWith(QStringLiteral("Chantilly")) || fontName.contains(QStringLiteral("BibleScr"))) {
        license[QStringLiteral("type")] = QStringLiteral("unknown"); /// unknown origin, but seems to be free for download
    } else if (fontName.startsWith(QStringLiteral("Bohemian-typewriter"))) {
        license[QStringLiteral("type")] = QStringLiteral("unknown"); /// free for non-commercial/personal use: http://www.dafont.com/bohemian-typewriter.font
    } else if (fontName.startsWith(QStringLiteral("Altera"))) {
        license[QStringLiteral("type")] = QStringLiteral("unknown"); /// Jacob King; free for personal use: http://www.jacobking.org/
    } else if (fontName.startsWith(QStringLiteral("Antenna"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Font Bureau
    } else if (fontName.contains(QStringLiteral("AkzidenzGrotesk")) || fontName.contains(QStringLiteral("Akzidenz Grotesk"))) {
        /// https://en.wikipedia.org/wiki/Akzidenz-Grotesk
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Akzidenz-Grotesk by H. Berthold AG
    } else if (fontName.contains(QStringLiteral("URWGrotesk"))) {
        /// https://www.myfonts.com/fonts/urw/grotesk/
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// 'URW Grotesk was designed exclusively for URW by Prof. Hermann Zapf in 1985'
    } else if (fontName.contains(QStringLiteral("VladimirScript"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// URW
    } else if (fontName.startsWith(QStringLiteral("Paperback"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// House Industries
    } else if (fontName.startsWith(QStringLiteral("Whitney"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Hoefler & Frere-Jones
    } else if (fontName.startsWith(QStringLiteral("DTL"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Dutch Type Library
    } else if (fontName.startsWith(QStringLiteral("Meta")) && (fontName.contains(QStringLiteral("Book")) || fontName.contains(QStringLiteral("Black")) || fontName.contains(QStringLiteral("Bold")) || fontName.contains(QStringLiteral("Normal")) || fontName.contains(QStringLiteral("Medium")))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// FontFont (FF)
    } else if (fontName.startsWith(QStringLiteral("DIN"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// FF?
    } else if (fontName.startsWith(QStringLiteral("Gotham")) || fontName.startsWith(QStringLiteral("NewLibrisSerif"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// fonts used by Forsakringskassan
    } else if (fontName.startsWith(QStringLiteral("Zapf")) || fontName.startsWith(QStringLiteral("Frutiger"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary");
    } else if (fontName.startsWith(QStringLiteral("Brandon"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); // HvD
    } else if (fontName.startsWith(QStringLiteral("Staat"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); // Timo Kuilder
    } else if (fontName.startsWith(QStringLiteral("Publico"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); // Paul Barnes and Christian Schwartz
    } else if (fontName.startsWith(QStringLiteral("ESRI"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// ESRI (maybe free-as-in-beer fonts?)
    } else if (fontName.startsWith(QStringLiteral("Interstate"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Font Bureau
    } else if (fontName.contains(QStringLiteral("BaskOldFace")) || fontName.startsWith(QStringLiteral("Baskerville"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft, URW, and Stephenson Blake
    } else if (fontName.startsWith(QStringLiteral("Microsoft")) || fontName.startsWith(QStringLiteral("MS-"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft
    } else if (fontName.startsWith(QStringLiteral("SegoeUI")) || fontName.startsWith(QStringLiteral("Segoe UI"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft
    } else if (fontName.contains(QStringLiteral("PGothic"))) {
        /// https://www.microsoft.com/typography/fonts/family.aspx?FID=335: 'MS PGothic is a Japanese font with proportional latin in the gothic (sans serif) style'
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft
    } else if (fontName.startsWith(QStringLiteral("Marlett")) || fontName.startsWith(QStringLiteral("Impact")) || fontName.startsWith(QStringLiteral("Comic Sans")) || fontName.contains(QStringLiteral("ComicSans")) || fontName.contains(QStringLiteral("Webdings")) || fontName.contains(QStringLiteral("Arial")) || fontName.startsWith(QStringLiteral("Verdana")) || fontName.contains(QStringLiteral("TimesNewRoman")) || fontName.startsWith(QStringLiteral("Times New Roman")) || fontName.contains(QStringLiteral("CourierNew")) || fontName.startsWith(QStringLiteral("Courier New")) || fontName.contains(QStringLiteral("Georgia")) || fontName == QStringLiteral("Symbol")) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft
    } else if (fontName.startsWith(QStringLiteral("Nyala")) || fontName.contains(QStringLiteral("Sylfaen")) || fontName.contains(QStringLiteral("BookAntiqua")) || fontName.contains(QStringLiteral("Lucinda")) || fontName.contains(QStringLiteral("Trebuchet")) || fontName.startsWith(QStringLiteral("Franklin Gothic")) || fontName.contains(QStringLiteral("FranklinGothic")) || fontName.startsWith(QStringLiteral("Century Schoolbook")) || fontName.contains(QStringLiteral("CenturySchoolbook"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft
    } else if (fontName.startsWith(QStringLiteral("MS Mincho")) || fontName.startsWith(QStringLiteral("MS-Mincho")) || fontName.startsWith(QStringLiteral("MSMincho"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft/Ricoh/Ryobi Imagix
    } else if (fontName.startsWith(QStringLiteral("SimSun"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft/ZHONGYI
    } else if (fontName.startsWith(QStringLiteral("Calibri")) || fontName.startsWith(QStringLiteral("CALIBRI")) || fontName.startsWith(QStringLiteral("Cambria")) || fontName.startsWith(QStringLiteral("Constantia")) || fontName.startsWith(QStringLiteral("Candara")) || fontName.startsWith(QStringLiteral("Corbel")) || fontName.startsWith(QStringLiteral("Consolas"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Microsoft ClearType Font Collection
    } else if (fontName.startsWith(QStringLiteral("Papyrus")) || fontName.startsWith(QStringLiteral("Avenir")) || fontName.startsWith(QStringLiteral("Plantin")) || fontName.startsWith(QStringLiteral("MathematicalPi")) || fontName.startsWith(QStringLiteral("ClearfaceGothic")) || fontName.startsWith(QStringLiteral("Berling")) /* there may be an URW++ variant */ || fontName.startsWith(QStringLiteral("Granjon")) || fontName.startsWith(QStringLiteral("Sabon")) || fontName.startsWith(QStringLiteral("Folio")) || fontName.startsWith(QStringLiteral("Futura")) || fontName.startsWith(QStringLiteral("Soho")) || fontName.startsWith(QStringLiteral("Eurostile")) || fontName.startsWith(QStringLiteral("NewCenturySchlbk")) || fontName.startsWith(QStringLiteral("TradeGothic")) || fontName.startsWith(QStringLiteral("Univers")) || fontName.contains(QStringLiteral("Palatino"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Linotype
    } else if (fontName.endsWith(QStringLiteral("BT")) || fontName.contains(QStringLiteral("BT-"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Bitstream
    } else if (fontName.contains(QStringLiteral("Monospace821")) || fontName.contains(QStringLiteral("Swiss721")) || fontName.contains(QStringLiteral("Humanist777")) || fontName.contains(QStringLiteral("Dutch801")) || fontName.startsWith(QStringLiteral("Zurich"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Bitstream
    } else if (fontName.startsWith(QStringLiteral("BrushScript"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Bitstream
    } else if (fontName.contains(QStringLiteral("Helvetica")) && fontName.contains(QStringLiteral("Neue"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Neue Helvetica by Linotype
    } else if (fontName.startsWith(QStringLiteral("Tahoma")) || fontName.contains(QStringLiteral("Helvetica")) || fontName.contains(QStringLiteral("Wingdings"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary");
    } else if (fontName.startsWith(QStringLiteral("Bookman-")) /* not "BookmanOldStyle"? */ || fontName.startsWith(QStringLiteral("SymbolMT")) || fontName.startsWith(QStringLiteral("GillAltOneMT"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// MonoType's font as shipped with Windows
    } else if (fontName.startsWith(QStringLiteral("Monotype")) || fontName.startsWith(QStringLiteral("OceanSans")) || fontName.startsWith(QStringLiteral("BookmanOldStyle")) || fontName.startsWith(QStringLiteral("Centaur")) || fontName.startsWith(QStringLiteral("Calisto")) || fontName.startsWith(QStringLiteral("CenturyGothic")) || fontName.startsWith(QStringLiteral("Bembo")) || fontName.startsWith(QStringLiteral("GillSans")) ||  fontName.startsWith(QStringLiteral("Rockwell")) || fontName.startsWith(QStringLiteral("Lucida")) || fontName.startsWith(QStringLiteral("Perpetua"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// MonoType
    } else if (fontName.startsWith(QStringLiteral("KunstlerScript")) || fontName.startsWith(QStringLiteral("AmericanTypewriter")) || fontName.startsWith(QStringLiteral("ACaslon")) || fontName.startsWith(QStringLiteral("AGaramond")) || fontName.startsWith(QStringLiteral("GaramondPremrPro")) || fontName.contains(QStringLiteral("EuroSans")) || fontName.startsWith(QStringLiteral("Minion")) || fontName.startsWith(QStringLiteral("Myriad"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Adobe
    } else if (fontName.startsWith(QStringLiteral("Itc")) || fontName.startsWith(QStringLiteral("ITC")) || fontName.endsWith(QStringLiteral("ITC")) || fontName.startsWith(QStringLiteral("AvantGarde")) || fontName.contains(QStringLiteral("Officina")) || fontName.contains(QStringLiteral("Kabel")) || fontName.contains(QStringLiteral("Cheltenham"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// ITC
    } else if (fontName.contains(QStringLiteral("BellGothic"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Mergenthaler Linotype/AT&T
    } else if (fontName.startsWith(QStringLiteral("StoneSans"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// ITC or Linotype?
    } else if (fontName.startsWith(QStringLiteral("TheSans"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Lucas
    } else if (fontName.startsWith(QStringLiteral("UU-"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Commercial font used by University of Uppsala
    } else if (fontName.endsWith(QStringLiteral("LiU"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// Commercial font used by University of Linkoping
    } else if (fontName.startsWith(QStringLiteral("Bookman Old Style")) || fontName.startsWith(QStringLiteral("Gill Sans"))) {
        license[QStringLiteral("type")] = QStringLiteral("proprietary"); /// multiple alternatives
    } else
        license[QStringLiteral("type")] = QStringLiteral("unknown");
    /// The following font names are ambiguous:
    /// - Courier
    /// - Garamond
    /// - Franklin Gothic
    /// - Symbol
    /// More fonts?

    QString bName = fontName;
    bName.remove(QStringLiteral("#20")).remove(QStringLiteral("\\s")).remove(QStringLiteral(".")).remove(QStringLiteral("_")).remove(QStringLiteral("/"));
    bool bNameChanged = true;
    while (bNameChanged) {
        const QString bNameOriginal = bName;
        bName = bName.trimmed();
        static const QStringList suffixes = QStringList()
                                            << QStringLiteral("BOLD") << QStringLiteral("ITALIC")
                                            << QStringLiteral("-KursivHalbfett") << QStringLiteral("-Kursiv")
                                            << QStringLiteral("OsF") << QStringLiteral("PS") << QStringLiteral("BE") << QStringLiteral("PSMT") << QStringLiteral("MS") << QStringLiteral("SC") << QStringLiteral("LT") << QStringLiteral("LF") << QStringLiteral("-BT") << QStringLiteral("BT") << QStringLiteral("Bk")
                                            << QStringLiteral("-Normal") << QStringLiteral("-Book") << QStringLiteral("-Md") << QStringLiteral("-Plain") << QStringLiteral("-Medium") << QStringLiteral("-MediumCond") << QStringLiteral("-Medi") << QStringLiteral("-MediumItalic") << QStringLiteral("-Semibold") << QStringLiteral("-SmbdIt") << QStringLiteral("-SemiCn") << QStringLiteral("-SemiCnIt") << QStringLiteral("-SemiboldSemiCn") << QStringLiteral("-Caps") << QStringLiteral("-Roman") << QStringLiteral("-Roma") << QStringLiteral("-Regular") << QStringLiteral("-Regu") << QStringLiteral("-DisplayRegular")
                                            << QStringLiteral("-Demi") << QStringLiteral("-Blk") << QStringLiteral("-Black") << QStringLiteral("-BlackIt") << QStringLiteral("-Blac") << QStringLiteral("Bla") << QStringLiteral("-BlackSemiCn") << QStringLiteral("-BlackSemiCnIt") << QStringLiteral("-Ultra") << QStringLiteral("-Extra") << QStringLiteral("-ExtraBold") << QStringLiteral("Obl") << QStringLiteral("-Hv") << QStringLiteral("-HvIt") << QStringLiteral("-Heavy") << QStringLiteral("-HeavyCond") << QStringLiteral("-Heav") << QStringLiteral("-BoldIt") << QStringLiteral("-BoldCn") << QStringLiteral("-BoldItal") << QStringLiteral("-BoldItalicB") << QStringLiteral("-BdIt") << QStringLiteral("-Bd") << QStringLiteral("-It")
                                            << QStringLiteral("-Condensed") << QStringLiteral("-Light") << QStringLiteral("-LightSemiCn") << QStringLiteral("-LightSemiCnIt") << QStringLiteral("-Ligh") << QStringLiteral("-Lt") << QStringLiteral("-Slant") << QStringLiteral("-LightCond") << QStringLiteral("Lig") << QStringLiteral("-Narrow") << QStringLiteral("-DmCn") << QStringLiteral("-BoldSemiCnIt") << QStringLiteral("-BoldSemiCn")
                                            << QStringLiteral("-BlackAlternate") << QStringLiteral("-BoldAlternate")
                                            << QStringLiteral("Ext") << QStringLiteral("Narrow") << QStringLiteral("SWA") << QStringLiteral("Std") << QStringLiteral("-Identity-H") << QStringLiteral("-DTC") << QStringLiteral("CE");
        for (const QString &suffix : suffixes) {
            if (bName.endsWith(suffix))
                bName = bName.left(bName.length() - suffix.length());
        }
        static const QVector<QRegExp> suffixesRegExp = QVector<QRegExp>()
                << QRegExp(QStringLiteral("^[1-9][0-9]+E[a-f0-9]{2,5}"))
                << QRegExp(QStringLiteral("(-[0-9])+$"))
                << QRegExp(QStringLiteral("[~][0-9a-f]+$"))
                << QRegExp(QStringLiteral("-Extend\\.[0-9]+$"))
                << QRegExp(QStringLiteral("(Fet|Kursiv)[0-9]+$"))
                << QRegExp(QStringLiteral("[,-]?(Ital(ic)?|Oblique|Black|Bol(dB?)?)$"))
                << QRegExp(QStringLiteral("[,-](BdCn|SC)[0-9]*$")) << QRegExp(QStringLiteral("[,-][A-Z][0-9]$")) << QRegExp(QStringLiteral("_[0-9]+$"))
                << QRegExp(QStringLiteral("[+][A-Z]+$"))
                << QRegExp(QStringLiteral("[*][0-9]+$"));
        for (const QRegExp &suffix : suffixesRegExp) {
            bName.remove(suffix);
        }
        static const QRegExp specialCaseSuffix = QRegExp(QStringLiteral("([a-z])(MT|T)$"));
        bName.replace(specialCaseSuffix, QStringLiteral("\\1"));
        static const QRegExp teXFonts = QRegExp(QStringLiteral("^((CM|SF|MS)[A-Z]+|(cm)[a-z]+|wasy|stmary|LM(Sans|Roman|Typewriter|Slanted|Math|Mono)[a-zA-Z]*)([0-9]+)$"));
        bName.replace(teXFonts, QStringLiteral("\\1"));
        static const QRegExp adobePrefix = QRegExp(QStringLiteral("^A(Caslon|Garamond)"));
        bName.replace(adobePrefix, QStringLiteral("\\1"));
        bName.replace(QStringLiteral("CambriaMath"), QStringLiteral("Cambria"));
        bName.replace(QStringLiteral("MetaTabell"), QStringLiteral("Meta"));
        bName.replace(QStringLiteral("MetaText"), QStringLiteral("Meta"));
        static const QVector<QPair<QString, QString> > microsoftNamesWithSpaces = QVector<QPair<QString, QString> >() << QPair<QString, QString>(QStringLiteral("Times New Roman"), QStringLiteral("TimesNewRoman")) << QPair<QString, QString>(QStringLiteral("Courier New"), QStringLiteral("CourierNew")) << QPair<QString, QString>(QStringLiteral("Comic Sans"), QStringLiteral("ComicSans"));
        for (QVector<QPair<QString, QString> >::ConstIterator it = microsoftNamesWithSpaces.constBegin(); it != microsoftNamesWithSpaces.constEnd(); ++it)
            bName.replace(it->first, it->second);
        static const QRegExp sixLettersPlusPrefix = QRegExp(QStringLiteral("^([A-Z]{6}\\+[_]?)([a-zA-Z0-9]{3,})"));
        if (sixLettersPlusPrefix.indexIn(bName) == 0)
            bName = bName.mid(sixLettersPlusPrefix.cap(1).length());
        if (bName.length() > 3 && bName[0] == QChar('*'))
            bName = bName.mid(1);
        bNameChanged = bName != bNameOriginal;
    }
    bName.remove(QStringLiteral("+"));
    if (bName.isEmpty()) bName = fontName; ///< Upps, removed too many characters?
    beautifiedName[QStringLiteral("")] = DocScan::xmlify(bName);

    QString text = typeName.toLower();
    if (text.indexOf(QStringLiteral("truetype")) >= 0)
        technology[QStringLiteral("type")] = QStringLiteral("truetype");
    else if (text.indexOf(QStringLiteral("type1")) >= 0)
        technology[QStringLiteral("type")] = QStringLiteral("type1");
    else if (text.indexOf(QStringLiteral("type3")) >= 0)
        technology[QStringLiteral("type")] = QStringLiteral("type3");

    const QString result = DocScan::formatMap(QStringLiteral("name"), name) + DocScan::formatMap(QStringLiteral("beautified"), beautifiedName) + DocScan::formatMap(QStringLiteral("technology"), technology) + DocScan::formatMap(QStringLiteral("license"), license);

    return result;
}

QString Guessing::programToXML(const QString &program) {
    const QString text = program.toLower();
    QHash<QString, QString> xml;
    xml[QStringLiteral("")] = program;
    bool checkOOoVersion = false;

    if (text.indexOf(QStringLiteral("dvips")) >= 0) {
        static const QRegExp radicaleyeVersion("\\b\\d+\\.\\d+[a-z]*\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("radicaleye");
        if (radicaleyeVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = radicaleyeVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("ghostscript")) >= 0) {
        static const QRegExp ghostscriptVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("artifex");
        xml[QStringLiteral("product")] = QStringLiteral("ghostscript");
        if (ghostscriptVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = ghostscriptVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("cairo "))) {
        static const QRegExp cairoVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("cairo");
        xml[QStringLiteral("product")] = QStringLiteral("cairo");
        if (cairoVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = cairoVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("pdftex")) >= 0) {
        static const QRegExp pdftexVersion("\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("pdftex");
        xml[QStringLiteral("product")] = QStringLiteral("pdftex");
        if (pdftexVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdftexVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("xetex")) >= 0) {
        static const QRegExp xetexVersion("\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("xetex");
        xml[QStringLiteral("product")] = QStringLiteral("xetex");
        if (xetexVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = xetexVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("latex")) >= 0) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("latex");
        xml[QStringLiteral("product")] = QStringLiteral("latex");
    } else if (text.indexOf(QStringLiteral("dvipdfm")) >= 0) {
        static const QRegExp dvipdfmVersion("\\b\\d+(\\.\\d+)+[a-z]*\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("dvipdfm");
        xml[QStringLiteral("product")] = QStringLiteral("dvipdfm");
        if (dvipdfmVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = dvipdfmVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("tex output")) >= 0) {
        static const QRegExp texVersion("\\b\\d+([.:]\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("tex");
        xml[QStringLiteral("product")] = QStringLiteral("tex");
        if (texVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = texVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("koffice")) >= 0) {
        static const QRegExp kofficeVersion("/(d+([.]\\d+)*)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("kde");
        xml[QStringLiteral("product")] = QStringLiteral("koffice");
        if (kofficeVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = kofficeVersion.cap(1);
    } else if (text.indexOf(QStringLiteral("calligra")) >= 0) {
        static const QRegExp calligraVersion("/(d+([.]\\d+)*)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("kde");
        xml[QStringLiteral("product")] = QStringLiteral("calligra");
        if (calligraVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = calligraVersion.cap(1);
    } else if (text.indexOf(QStringLiteral("abiword")) >= 0) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("abisource");
        xml[QStringLiteral("product")] = QStringLiteral("abiword");
    } else if (text.indexOf(QStringLiteral("office_one")) >= 0) {
        checkOOoVersion = true;
        xml[QStringLiteral("product")] = QStringLiteral("office_one");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
    } else if (text.indexOf(QStringLiteral("infraoffice")) >= 0) {
        checkOOoVersion = true;
        xml[QStringLiteral("product")] = QStringLiteral("infraoffice");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
    } else if (text.indexOf(QStringLiteral("aksharnaveen")) >= 0) {
        checkOOoVersion = true;
        xml[QStringLiteral("product")] = QStringLiteral("aksharnaveen");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
    } else if (text.indexOf(QStringLiteral("redoffice")) >= 0) {
        checkOOoVersion = true;
        xml[QStringLiteral("manufacturer")] = QStringLiteral("china");
        xml[QStringLiteral("product")] = QStringLiteral("redoffice");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
    } else if (text.indexOf(QStringLiteral("sun_odf_plugin")) >= 0) {
        checkOOoVersion = true;
        xml[QStringLiteral("manufacturer")] = QStringLiteral("oracle");
        xml[QStringLiteral("product")] = QStringLiteral("odfplugin");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
    } else if (text.indexOf(QStringLiteral("libreoffice")) >= 0) {
        checkOOoVersion = true;
        xml[QStringLiteral("manufacturer")] = QStringLiteral("tdf");
        xml[QStringLiteral("product")] = QStringLiteral("libreoffice");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
    }  else if (text.indexOf(QStringLiteral("lotus symphony")) >= 0) {
        static const QRegExp lotusSymphonyVersion("Symphony (\\d+(\\.\\d+)*)");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("ibm");
        xml[QStringLiteral("product")] = QStringLiteral("lotus-symphony");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
        if (lotusSymphonyVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = lotusSymphonyVersion.cap(1);
    }  else if (text.indexOf(QStringLiteral("Lotus_Symphony")) >= 0) {
        checkOOoVersion = true;
        xml[QStringLiteral("manufacturer")] = QStringLiteral("ibm");
        xml[QStringLiteral("product")] = QStringLiteral("lotus-symphony");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
    } else if (text.indexOf(QStringLiteral("openoffice")) >= 0) {
        checkOOoVersion = true;
        if (text.indexOf(QStringLiteral("staroffice")) >= 0) {
            xml[QStringLiteral("manufacturer")] = QStringLiteral("oracle");
            xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
            xml[QStringLiteral("product")] = QStringLiteral("staroffice");
        } else if (text.indexOf(QStringLiteral("broffice")) >= 0) {
            xml[QStringLiteral("product")] = QStringLiteral("broffice");
            xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
        } else if (text.indexOf(QStringLiteral("neooffice")) >= 0) {
            xml[QStringLiteral("manufacturer")] = QStringLiteral("planamesa");
            xml[QStringLiteral("product")] = QStringLiteral("neooffice");
            xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
        } else {
            xml[QStringLiteral("manufacturer")] = QStringLiteral("oracle");
            xml[QStringLiteral("product")] = QStringLiteral("openoffice");
        }
    } else if (text == QStringLiteral("writer") || text == QStringLiteral("calc") || text == QStringLiteral("impress")) {
        /// for Creator/Editor string
        xml[QStringLiteral("manufacturer")] = QStringLiteral("oracle;tdf");
        xml[QStringLiteral("product")] = QStringLiteral("openoffice;libreoffice");
        xml[QStringLiteral("based-on")] = QStringLiteral("openoffice");
    } else if (text.startsWith(QStringLiteral("pdfscanlib "))) {
        static const QRegExp pdfscanlibVersion("v(\\d+(\\.\\d+)+)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("kodak?");
        xml[QStringLiteral("product")] = QStringLiteral("pdfscanlib");
        if (pdfscanlibVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdfscanlibVersion.cap(1);
    } else if (text.indexOf(QStringLiteral("framemaker")) >= 0) {
        static const QRegExp framemakerVersion("\\b\\d+(\\.\\d+)+(\\b|\\.|p\\d+)");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("framemaker");
        if (framemakerVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = framemakerVersion.cap(0);
    } else if (text.contains(QStringLiteral("distiller"))) {
        static const QRegExp distillerVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("distiller");
        if (distillerVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = distillerVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("pdflib plop"))) {
        static const QRegExp plopVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("pdflib");
        xml[QStringLiteral("product")] = QStringLiteral("plop");
        if (plopVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = plopVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("pdflib"))) {
        static const QRegExp pdflibVersion("\\b\\d+(\\.[0-9p]+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("pdflib");
        xml[QStringLiteral("product")] = QStringLiteral("pdflib");
        if (pdflibVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdflibVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("pdf library")) >= 0) {
        static const QRegExp pdflibraryVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("pdflibrary");
        if (pdflibraryVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdflibraryVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("pdfwriter")) >= 0) {
        static const QRegExp pdfwriterVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("pdfwriter");
        if (pdfwriterVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdfwriterVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("easypdf")) >= 0) {
        static const QRegExp easypdfVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("bcl");
        xml[QStringLiteral("product")] = QStringLiteral("easypdf");
        if (easypdfVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = easypdfVersion.cap(0);
    } else if (text.contains(QStringLiteral("pdfmaker"))) {
        static const QRegExp pdfmakerVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("pdfmaker");
        if (pdfmakerVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdfmakerVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("fill-in "))) {
        static const QRegExp fillInVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("textcenter");
        xml[QStringLiteral("product")] = QStringLiteral("fill-in");
        if (fillInVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = fillInVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("itext "))) {
        static const QRegExp iTextVersion("\\b((\\d+)(\\.\\d+)+)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("itext");
        xml[QStringLiteral("product")] = QStringLiteral("itext");
        if (iTextVersion.indexIn(text) >= 0) {
            xml[QStringLiteral("version")] = iTextVersion.cap(0);
            bool ok = false;
            const int majorVersion = iTextVersion.cap(2).toInt(&ok);
            if (ok && majorVersion > 0) {
                if (majorVersion <= 4)
                    xml[QStringLiteral("license")] = QStringLiteral("MPL;LGPL");
                else if (majorVersion >= 5)
                    xml[QStringLiteral("license")] = QStringLiteral("commercial;AGPLv3");
            }
        }
    } else if (text.startsWith(QStringLiteral("amyuni pdf converter "))) {
        static const QRegExp amyunitVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("amyuni");
        xml[QStringLiteral("product")] = QStringLiteral("pdfconverter");
        if (amyunitVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = amyunitVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("pdfout v")) >= 0) {
        static const QRegExp pdfoutVersion("v(\\d+(\\.\\d+)+)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("verypdf");
        xml[QStringLiteral("product")] = QStringLiteral("docconverter");
        if (pdfoutVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdfoutVersion.cap(1);
    } else if (text.indexOf(QStringLiteral("jaws pdf creator")) >= 0) {
        static const QRegExp pdfcreatorVersion("v(\\d+(\\.\\d+)+)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("jaws");
        xml[QStringLiteral("product")] = QStringLiteral("pdfcreator");
        if (pdfcreatorVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdfcreatorVersion.cap(1);
    } else if (text.startsWith(QStringLiteral("arbortext "))) {
        static const QRegExp arbortextVersion("\\d+(\\.\\d+)+)");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("ptc");
        xml[QStringLiteral("product")] = QStringLiteral("arbortext");
        if (arbortextVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = arbortextVersion.cap(0);
    } else if (text.contains(QStringLiteral("3b2"))) {
        static const QRegExp threeB2Version("\\d+(\\.[0-9a-z]+)+)");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("ptc");
        xml[QStringLiteral("product")] = QStringLiteral("3b2");
        if (threeB2Version.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = threeB2Version.cap(0);
    } else if (text.startsWith(QStringLiteral("3-heights"))) {
        static const QRegExp threeHeightsVersion("\\b\\d+(\\.\\d+)+)");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("pdftoolsag");
        xml[QStringLiteral("product")] = QStringLiteral("3-heights");
        if (threeHeightsVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = threeHeightsVersion.cap(0);
    } else if (text.contains(QStringLiteral("abcpdf"))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("websupergoo");
        xml[QStringLiteral("product")] = QStringLiteral("abcpdf");
    } else if (text.indexOf(QStringLiteral("primopdf")) >= 0) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("nitro");
        xml[QStringLiteral("product")] = QStringLiteral("primopdf");
        xml[QStringLiteral("based-on")] = QStringLiteral("nitropro");
    } else if (text.indexOf(QStringLiteral("nitro")) >= 0) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("nitro");
        xml[QStringLiteral("product")] = QStringLiteral("nitropro");
    } else if (text.indexOf(QStringLiteral("pdffactory")) >= 0) {
        static const QRegExp pdffactoryVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("softwarelabs");
        xml[QStringLiteral("product")] = QStringLiteral("pdffactory");
        if (pdffactoryVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdffactoryVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("ibex pdf"))) {
        static const QRegExp ibexVersion("\\b\\d+(\\.\\[0-9/]+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("visualprogramming");
        xml[QStringLiteral("product")] = QStringLiteral("ibexpdfcreator");
        if (ibexVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = ibexVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("arc/info")) || text.startsWith(QStringLiteral("arcinfo"))) {
        static const QRegExp arcinfoVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("esri");
        xml[QStringLiteral("product")] = QStringLiteral("arcinfo");
        if (arcinfoVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = arcinfoVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("paperport "))) {
        static const QRegExp paperportVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("nuance");
        xml[QStringLiteral("product")] = QStringLiteral("paperport");
        if (paperportVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = paperportVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("indesign")) >= 0) {
        static const QRegExp indesignVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("indesign");
        if (indesignVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = indesignVersion.cap(0);
        else {
            static const QRegExp csVersion(QStringLiteral("\\bCS(\\d*)\\b"));
            if (csVersion.indexIn(text) >= 0) {
                bool ok = false;
                double versionNumber = csVersion.cap(1).toDouble(&ok);
                if (csVersion.cap(0) == QStringLiteral("CS"))
                    xml[QStringLiteral("version")] = QStringLiteral("3.0");
                else if (ok && versionNumber > 1) {
                    versionNumber += 2;
                    xml[QStringLiteral("version")] = QString::number(versionNumber, 'f', 1);
                }
            }
        }
    } else if (text.indexOf(QStringLiteral("illustrator")) >= 0) {
        static const QRegExp illustratorVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("illustrator");
        if (illustratorVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = illustratorVersion.cap(0);
        else {
            static const QRegExp csVersion(QStringLiteral("\\bCS(\\d*)\\b"));
            if (csVersion.indexIn(text) >= 0) {
                bool ok = false;
                double versionNumber = csVersion.cap(1).toDouble(&ok);
                if (csVersion.cap(0) == QStringLiteral("CS"))
                    xml[QStringLiteral("version")] = QStringLiteral("11.0");
                else if (ok && versionNumber > 1) {
                    versionNumber += 10;
                    xml[QStringLiteral("version")] = QString::number(versionNumber, 'f', 1);
                }
            }
        }
    } else if (text.indexOf(QStringLiteral("pagemaker")) >= 0) {
        static const QRegExp pagemakerVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("pagemaker");
        if (pagemakerVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pagemakerVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("acrobat capture")) >= 0) {
        static const QRegExp acrobatCaptureVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("acrobatcapture");
        if (acrobatCaptureVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = acrobatCaptureVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("acrobat pro")) >= 0) {
        static const QRegExp acrobatProVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("acrobatpro");
        if (acrobatProVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = acrobatProVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("acrobat")) >= 0) {
        static const QRegExp acrobatVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("acrobat");
        if (acrobatVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = acrobatVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("livecycle")) >= 0) {
        static const QRegExp livecycleVersion("\\b\\d+(\\.\\d+)+[a-z]?\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        int regExpPos;
        if ((regExpPos = livecycleVersion.indexIn(text)) >= 0)
            xml[QStringLiteral("version")] = livecycleVersion.cap(0);
        if (regExpPos <= 0)
            regExpPos = 1024;
        QString product = text;
        xml[QStringLiteral("product")] = product.left(regExpPos - 1).remove(QStringLiteral("adobe")).remove(livecycleVersion.cap(0)).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.startsWith(QStringLiteral("adobe photoshop elements"))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("photoshopelements");
    } else if (text.startsWith(QStringLiteral("adobe photoshop"))) {
        static const QRegExp photoshopVersion("\\bCS|(CS)?\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        xml[QStringLiteral("product")] = QStringLiteral("photoshop");
        if (photoshopVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = photoshopVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("adobe")) >= 0) {
        /// some unknown Adobe product
        static const QRegExp adobeVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");
        if (adobeVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = adobeVersion.cap(0);
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("adobe")).remove(adobeVersion.cap(0)).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.contains(QStringLiteral("pages"))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("apple");
        xml[QStringLiteral("product")] = QStringLiteral("pages");
    } else if (text.contains(QStringLiteral("keynote"))) {
        static const QRegExp keynoteVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("apple");
        xml[QStringLiteral("product")] = QStringLiteral("keynote");
        if (keynoteVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = keynoteVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("quartz")) >= 0) {
        static const QRegExp quartzVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("apple");
        xml[QStringLiteral("product")] = QStringLiteral("quartz");
        if (quartzVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = quartzVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("pscript5.dll")) >= 0 || text.indexOf(QStringLiteral("pscript.dll")) >= 0) {
        static const QRegExp pscriptVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("microsoft");
        xml[QStringLiteral("product")] = QStringLiteral("pscript");
        xml[QStringLiteral("opsys")] = QStringLiteral("windows");
        if (pscriptVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pscriptVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("quarkxpress")) >= 0) {
        static const QRegExp quarkxpressVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("quark");
        xml[QStringLiteral("product")] = QStringLiteral("xpress");
        if (quarkxpressVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = quarkxpressVersion.cap(0);
    } else if (text.indexOf(QStringLiteral("pdfcreator")) >= 0) {
        static const QRegExp pdfcreatorVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("pdfforge");
        xml[QStringLiteral("product")] = QStringLiteral("pdfcreator");
        xml[QStringLiteral("opsys")] = QStringLiteral("windows");
        if (pdfcreatorVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdfcreatorVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("stamppdf batch"))) {
        static const QRegExp stamppdfbatchVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("appligent");
        xml[QStringLiteral("product")] = QStringLiteral("stamppdfbatch");
        if (stamppdfbatchVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = stamppdfbatchVersion.cap(0);
    } else if (text.startsWith(QStringLiteral("xyenterprise "))) {
        static const QRegExp xyVersion("\b\\d+(\\.\\[0-9a-z])+)( patch \\S*\\d)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("dakota");
        xml[QStringLiteral("product")] = QStringLiteral("xyenterprise");
        if (xyVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = xyVersion.cap(1);
    } else if (text.startsWith(QStringLiteral("edocprinter "))) {
        static const QRegExp edocprinterVersion("ver (\\d+(\\.\\d+)+)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("itek");
        xml[QStringLiteral("product")] = QStringLiteral("edocprinter");
        if (edocprinterVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = edocprinterVersion.cap(1);
    } else if (text.startsWith(QStringLiteral("pdf code "))) {
        static const QRegExp pdfcodeVersion("\\b(\\d{8}}|d+(\\.\\d+)+)\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("europeancommission");
        xml[QStringLiteral("product")] = QStringLiteral("pdfcode");
        if (pdfcodeVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = pdfcodeVersion.cap(1);
    } else if (text.indexOf(QStringLiteral("pdf printer")) >= 0) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("bullzip");
        xml[QStringLiteral("product")] = QStringLiteral("pdfprinter");
    } else if (text.contains(QStringLiteral("aspose")) && text.contains(QStringLiteral("words"))) {
        static const QRegExp asposewordsVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("aspose");
        xml[QStringLiteral("product")] = QStringLiteral("aspose.words");
        if (asposewordsVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = asposewordsVersion.cap(0);
    } else if (text.contains(QStringLiteral("arcmap"))) {
        static const QRegExp arcmapVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("esri");
        xml[QStringLiteral("product")] = QStringLiteral("arcmap");
        if (arcmapVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = arcmapVersion.cap(0);
    } else if (text.contains(QStringLiteral("ocad"))) {
        static const QRegExp ocadVersion("\\b\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("ocad");
        xml[QStringLiteral("product")] = QStringLiteral("ocad");
        if (ocadVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = ocadVersion.cap(0);
    } else if (text.contains(QStringLiteral("gnostice"))) {
        static const QRegExp gnosticeVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("gnostice");
        if (gnosticeVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = gnosticeVersion.cap(0);
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("gnostice")).remove(gnosticeVersion.cap(0)).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.contains(QStringLiteral("canon"))) {
        static const QRegExp canonVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("canon");
        if (canonVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = canonVersion.cap(0);
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("canon")).remove(canonVersion.cap(0)).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.startsWith(QStringLiteral("creo"))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("creo");
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("creo")).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.contains(QStringLiteral("apogee"))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("agfa");
        xml[QStringLiteral("product")] = QStringLiteral("apogee");
    } else if (text.contains(QStringLiteral("ricoh"))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("ricoh");
        const int i = text.indexOf(QStringLiteral("aficio"));
        if (i >= 0)
            xml[QStringLiteral("product")] = text.mid(i).replace(QLatin1Char(' '), QString::null);
    } else if (text.contains(QStringLiteral("toshiba")) || text.contains(QStringLiteral("mfpimglib"))) {
        static const QRegExp toshibaVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("toshiba");
        if (toshibaVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = toshibaVersion.cap(0);
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("toshiba")).remove(toshibaVersion.cap(0)).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.startsWith(QStringLiteral("hp ")) || text.startsWith(QStringLiteral("hewlett packard "))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("hewlettpackard");
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("hp ")).remove(QStringLiteral("hewlett packard")).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.startsWith(QStringLiteral("xerox "))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("xerox");
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("xerox ")).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.startsWith(QStringLiteral("kodak "))) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("kodak");
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("kodak ")).remove(QStringLiteral("scanner: ")).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.contains(QStringLiteral("konica")) || text.contains(QStringLiteral("minolta"))) {
        static const QRegExp konicaMinoltaVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("konica;minolta");
        if (konicaMinoltaVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = konicaMinoltaVersion.cap(0);
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("konica")).remove(QStringLiteral("minolta")).remove(konicaMinoltaVersion.cap(0)).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.contains(QStringLiteral("corel"))) {
        static const QRegExp corelVersion("\\b[v]?\\d+(\\.\\d+)+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("corel");
        if (corelVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = corelVersion.cap(0);
        QString product = text;
        xml[QStringLiteral("product")] = product.remove(QStringLiteral("corel")).remove(corelVersion.cap(0)).remove(QStringLiteral(" ")) + QLatin1Char('?');
    } else if (text.contains(QStringLiteral("scansoft pdf create"))) {
        static const QRegExp scansoftVersion("\\b([a-zA-Z]+[ ])?[A-Za-z0-9]+\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("scansoft");
        xml[QStringLiteral("product")] = QStringLiteral("pdfcreate");
        if (scansoftVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = scansoftVersion.cap(0);
    } else if (text.contains(QStringLiteral("alivepdf"))) {
        static const QRegExp alivepdfVersion("\\b\\d+(\\.\\d+)+( RC)?\\b");
        xml[QStringLiteral("manufacturer")] = QStringLiteral("thibault.imbert");
        xml[QStringLiteral("product")] = QStringLiteral("alivepdf");
        if (alivepdfVersion.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = alivepdfVersion.cap(0);
        xml[QStringLiteral("opsys")] = QStringLiteral("flash");
    } else if (text == QStringLiteral("google")) {
        xml[QStringLiteral("manufacturer")] = QStringLiteral("google");
        xml[QStringLiteral("product")] = QStringLiteral("docs");
    } else if (!text.contains(QStringLiteral("words"))) {
        static const QRegExp microsoftProducts("powerpoint|excel|word|outlook|visio|access");
        static const QRegExp microsoftVersion("\\b(starter )?(20[01][0-9]|1?[0-9]\\.[0-9]+|9[5-9])\\b");
        if (microsoftProducts.indexIn(text) >= 0) {
            xml[QStringLiteral("manufacturer")] = QStringLiteral("microsoft");
            xml[QStringLiteral("product")] = microsoftProducts.cap(0);
            if (!xml.contains(QStringLiteral("version")) && microsoftVersion.indexIn(text) >= 0)
                xml[QStringLiteral("version")] = microsoftVersion.cap(2);
            if (!xml.contains(QStringLiteral("subversion")) && !microsoftVersion.cap(1).isEmpty())
                xml[QStringLiteral("subversion")] = microsoftVersion.cap(1);

            if (text.contains(QStringLiteral("Macintosh")) || text.contains(QStringLiteral("Mac OS X")))
                xml[QStringLiteral("opsys")] = QStringLiteral("macosx");
            else
                xml[QStringLiteral("opsys")] = QStringLiteral("windows?");
        }
    }

    if (checkOOoVersion) {
        /// Looks like "Win32/2.3.1"
        static const QRegExp OOoVersion1("[a-z]/(\\d(\\.\\d+)+)(_beta|pre)?[$a-z]", Qt::CaseInsensitive);
        if (OOoVersion1.indexIn(text) >= 0)
            xml[QStringLiteral("version")] = OOoVersion1.cap(1);
        else {
            /// Fallback: conventional version string like "3.0"
            static const QRegExp OOoVersion2("\\b(\\d+(\\.\\d+)+)\\b", Qt::CaseInsensitive);
            if (OOoVersion2.indexIn(text) >= 0)
                xml[QStringLiteral("version")] = OOoVersion2.cap(1);
        }

        if (text.indexOf(QStringLiteral("unix")) >= 0)
            xml[QStringLiteral("opsys")] = QStringLiteral("generic-unix");
        else if (text.indexOf(QStringLiteral("linux")) >= 0)
            xml[QStringLiteral("opsys")] = QStringLiteral("linux");
        else if (text.indexOf(QStringLiteral("win32")) >= 0)
            xml[QStringLiteral("opsys")] = QStringLiteral("windows");
        else if (text.indexOf(QStringLiteral("solaris")) >= 0)
            xml[QStringLiteral("opsys")] = QStringLiteral("solaris");
        else if (text.indexOf(QStringLiteral("freebsd")) >= 0)
            xml[QStringLiteral("opsys")] = QStringLiteral("bsd");
    }

    if (!xml.contains(QStringLiteral("manufacturer")) && (text.contains(QStringLiteral("adobe")) || text.contains(QStringLiteral("acrobat"))))
        xml[QStringLiteral("manufacturer")] = QStringLiteral("adobe");

    if (!xml.contains(QStringLiteral("opsys"))) {
        /// automatically guess operating system
        if (text.contains(QStringLiteral("macint")))
            xml[QStringLiteral("opsys")] = QStringLiteral("macosx");
        else if (text.contains(QStringLiteral("solaris")))
            xml[QStringLiteral("opsys")] = QStringLiteral("solaris");
        else if (text.contains(QStringLiteral("linux")))
            xml[QStringLiteral("opsys")] = QStringLiteral("linux");
        else if (text.contains(QStringLiteral("windows")) || text.contains(QStringLiteral("win32")) || text.contains(QStringLiteral("win64")))
            xml[QStringLiteral("opsys")] = QStringLiteral("windows");
    }

    const QString result = DocScan::formatMap(QStringLiteral("name"), xml);

    return result;
}
