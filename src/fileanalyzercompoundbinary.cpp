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

#include <QFile>
#include <QDebug>
#include <QtEndian>
#include <QFileInfo>
#include <QStringList>

#include <olestream.h>
#include <word97_helper.h>
#include <parser.h>
#include <fields.h>
#include <handlers.h>
#include <parserfactory.h>

#include "fileanalyzercompoundbinary.h"
#include "general.h"

inline QString string(const wvWare::UString &str)
{
    // Do a deep copy.  We used to not do that, but it lead to several
    // memory corruption bugs that were difficult to find.
    //
    // FIXME: Get rid of UString altogether and port wv2 to QString.
    return QString(reinterpret_cast<const QChar *>(str.data()), str.length());
}


class FileAnalyzerCompoundBinary::DocScanTextHandler: public wvWare::TextHandler
{
private:
    QString lidToLangCode(int lid) {
        switch (lid) {
        case 0x0400: return QLatin1String("-none-"); /// == 1024
        case 0x0401: return QLatin1String("-arabic-"); /// == 1025
        case 0x0402: return QLatin1String("bg"); /// == 1026
        case 0x0403: return QLatin1String("cat"); /// == 1027
        case 0x0404: return QLatin1String("cn"); /// == 1028
        case 0x0405: return QLatin1String("cs-CZ"); /// == 1029
        case 0x0406: return QLatin1String("da-DK"); /// == 1030
        case 0x0407: return QLatin1String("de-DE"); /// == 1031
        case 0x0408: return QLatin1String("gr"); /// == 1032
        case 0x0409: return QLatin1String("en-US"); /// == 1033
        case 0x040a: return QLatin1String("es"); /// == 1034
        case 0x040b: return QLatin1String("fi-FI"); /// == 1035
        case 0x040c: return QLatin1String("fr-FR"); /// == 1036
        case 0x040d: return QLatin1String("iw-IL"); /// == 1037
        case 0x040e: return QLatin1String("hu"); /// == 1038
        case 0x0410: return QLatin1String("it-IT"); /// == 1040
        case 0x0411: return QLatin1String("jp"); /// == 1041
        case 0x0412: return QLatin1String("kr"); /// == 1042
        case 0x0413: return QLatin1String("nl-NL"); /// == 1043
        case 0x0414: return QLatin1String("nb"); /// == 1044
        case 0x0415: return QLatin1String("pl"); /// == 1045
        case 0x0416: return QLatin1String("pt-BR"); /// == 1046
        case 0x0418: return QLatin1String("ro"); /// == 1048
        case 0x0419: return QLatin1String("ru-RU"); /// == 1049
        case 0x041b: return QLatin1String("sk"); /// == 1051
        case 0x041d: return QLatin1String("sv-SE"); /// == 1053
        case 0x041e: return QLatin1String("th"); /// == 1054
        case 0x041f: return QLatin1String("tr"); /// == 1055
        case 0x0421: return QLatin1String("id"); /// == 1057
        case 0x0424: return QLatin1String("sl"); /// == 1060
        case 0x042d: return QLatin1String("-basque-"); /// == 1069
        case 0x0804: return QLatin1String("cn");
        case 0x0809: return QLatin1String("en-GB");
        case 0x080a: return QLatin1String("es-ES"); ///< es-MX?
        case 0x0816: return QLatin1String("pt-PT");
        case 0x0c09: return QLatin1String("en-AU");
        case 0x0c0a: return QLatin1String("pt"); ///< ?
        case 0x0c0c: return QLatin1String("fr-CA"); ///< ?
        default: {
            QString language;
            language.setNum(lid, 16);
            return "0x" + language;
        }
        }
    }

    ResultContainer &resultContainer;

public:
    DocScanTextHandler(ResultContainer &result)
        : wvWare::TextHandler(), resultContainer(result) {
        // nothing
    }

    void runOfText(const wvWare::UString &text, wvWare::SharedPtr<const wvWare::Word97::CHP> chp) {
        QString qString = string(text);
        resultContainer.plainText += qString;
        if (resultContainer.language.isEmpty())
            resultContainer.language = lidToLangCode(chp->lidDefault);
    }

    void sectionStart(wvWare::SharedPtr<const wvWare::Word97::SEP> sep) {
        resultContainer.paperSizeWidth = sep->xaPage / 56.694;
        resultContainer.paperSizeHeight = sep->yaPage / 56.694;
    }
};

FileAnalyzerCompoundBinary::FileAnalyzerCompoundBinary(QObject *parent)
    : FileAnalyzerAbstract(parent), m_isAlive(false)
{
    // nothing
}

bool FileAnalyzerCompoundBinary::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerCompoundBinary::analyzeFiB(wvWare::Word97::FIB &fib, ResultContainer &result)
{

    /// try to guess operating system based on header information
    // TODO: What value is set by non-Microsoft editors on e.g. Linux?
    QString opsys;
    switch (fib.envr) {
    case 0: result.opSys = QLatin1String("windows"); break;
    case 1: result.opSys = QLatin1String("mac"); break;
    default: result.opSys = QString("unknown=%1").arg(fib.envr);
    }

    getVersion(fib.nFib, result.versionNumber, result.versionText);
    getEditor(fib.wMagicCreated, result.creatorText);
    getEditor(fib.wMagicRevised, result.revisorText);
}

bool FileAnalyzerCompoundBinary::getVersion(unsigned short nFib, int &versionNumber, QString &versionText)
{
    bool result = true;

    /// determine version format of this file
    switch (nFib) {
    case 0x0065: /// == 101
    case 0x0066: /// == 102
        versionNumber = 6;
        versionText = QLatin1String("Word 6.0");
        break;
    case 0x0067: /// == 103
        versionNumber = 6;
        versionText = QLatin1String("Word 6.0 for Macintosh");
        break;
    case 0x0068: /// == 104 FIXME other sources claim this is Word 6.0 for Macintosh
    case 0x0069: /// == 105
        versionNumber = 7;
        versionText = QLatin1String("Word 95");
        break;
    case 0x00c0:
    case 0x00c1:
    case 0x00c2:
        versionNumber = 8;
        versionText = QLatin1String("Word 97");
        break;
    case 0x00d9: /// == 217
        versionNumber = 9;
        versionText = QLatin1String("Word 2000");
        break;
    case 0x0101: /// == 257
        versionNumber = 10;
        versionText = QLatin1String("Word 2002 (XP)");
        break;
    case 0x010c: /// == 268
        versionNumber = 11;
        versionText = QLatin1String("Word 2003");
        break;
    default:
        versionNumber = 0;
        versionText = nFib == 0 ? QString::null : QLatin1String("nFib=") + QString::number(nFib, 16);
        result = false;
    }

    return result;
}

bool FileAnalyzerCompoundBinary::getEditor(unsigned short wMagic, QString &editorText)
{
    bool result = true;

    /// determine used editor
    switch (wMagic) {
    case 0x6a62:
        editorText = QLatin1String("Microsoft Word 97");
        break;
    case 0x626a:
        editorText = QLatin1String("Microsoft Word 98 for Macintosh");
        break;
    case 0x6143:
    case 0x6C6F: /// why two different numbers? Both are used in LibreOffice's and OpenOffice's source code
        editorText = QLatin1String("OpenOffice or LibreOffice");
        break;
    case 0xa5dc:
        editorText = QLatin1String("Microsoft Word 6.0/7.0");
        break;
    case 0xa5ec:
        editorText = QLatin1String("Microsoft Word 8.0");
        break;
    default:
        editorText = wMagic == 0 ? QString::null : QLatin1String("wMagic=") + QString::number(wMagic, 16);
        result = false;
    }

    return result;
}

void FileAnalyzerCompoundBinary::analyzeTable(wvWare::OLEStorage &storage, wvWare::Word97::FIB &fib, ResultContainer &result)
{
    wvWare::OLEStreamReader *table = storage.createStreamReader(fib.fWhichTblStm ? "1Table" : "0Table");
    if (table == NULL || !table->isValid()) {
        if (table != NULL)  delete table;
        return;
    }

    /// read meta information from table
    table->seek(fib.fcSttbfAssoc);
    wvWare::STTBF sttbf(fib.lid, table);
    int i = 0;
    for (wvWare::UString s = sttbf.firstString(); !s.isNull(); s = sttbf.nextString(), ++i) {
        if (s.isEmpty()) continue;

        switch (i) {
        case 2:
            result.title = s.ascii();
            break;
        case 3:
            result.subject = s.ascii();
            break;
        case 4:
            result.keywords = s.ascii();
            break;
        case 6:
            result.authorInitial = s.ascii();
            break;
        case 7:
            result.authorLast = s.ascii();
            break;
        }
    }

    delete table;
}

void FileAnalyzerCompoundBinary::analyzeWithParser(std::string &filename, ResultContainer &result)
{
    wvWare::SharedPtr<wvWare::Parser> parser(wvWare::ParserFactory::createParser(filename));
    if (parser != NULL) {
        if (parser->isOk()) {
            DocScanTextHandler *textHandler = new DocScanTextHandler(result);
            parser->setTextHandler(textHandler);

            if (parser->parse()) {
                const wvWare::Word97::DOP dop = parser->dop();

                result.dateCreation = QDate(1900 + dop.dttmCreated.yr, dop.dttmCreated.mon, dop.dttmCreated.dom);
                result.dateModification = QDate(1900 + dop.dttmRevised.yr, dop.dttmRevised.mon, dop.dttmRevised.dom);
                result.pageCount = dop.cPg;
                result.charCount = dop.cCh;
            }

            delete textHandler;
        }
    }

}

void FileAnalyzerCompoundBinary::analyzeFile(const QString &filename)
{
    m_isAlive = true;
    ResultContainer result;
    result.paperSizeWidth = 0;
    result.paperSizeHeight = 0;

    if (isRTFfile(filename)) {
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"RTF file disguising as DOC\" filename=\"%1\" />\n").arg(filename));
        m_isAlive = false;
        return;
    }

    std::string cppFilename = std::string(filename.toUtf8().constData());

    /// perform various file checks before starting the analysis
    wvWare::OLEStorage storage(cppFilename);
    if (!storage.open(wvWare::OLEStorage::ReadOnly)) {
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"OLEStorage cannot be opened\" filename=\"%1\" />\n").arg(filename));
        m_isAlive = false;
        return;
    }
    wvWare::OLEStreamReader *document = storage.createStreamReader("WordDocument");
    if (document == NULL || !document->isValid()) {
        if (document != NULL)  delete document;
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"Not a valid Word document\" filename=\"%1\" />\n").arg(filename));
        m_isAlive = false;
        return;
    }

    /// determine mimetype and write file analysis header
    QString mimetype = "application/octet-stream";
    if (filename.endsWith(".doc"))
        mimetype = "application/msword";

    /// get the FIB (File information block) which contains a lot of interesting information
    wvWare::Word97::FIB fib(document, true);
    analyzeFiB(fib, result);

    /// read meta information from table
    analyzeTable(storage, fib, result);

    /// analyze file with parser
    analyzeWithParser(cppFilename, result);

    QString logText = QString("<fileanalysis status=\"ok\" filename=\"%1\">\n").arg(DocScan::xmlify(filename));
    QString metaText = QLatin1String("<meta>\n");
    QString headerText = QLatin1String("<header>\n");

    /// file format including mime type and file format version
    metaText.append(QString(QLatin1String("<fileformat>\n<mimetype>%1</mimetype>\n<version major=\"%2\" minor=\"0\">%3</version>\n</fileformat>\n")).arg(mimetype).arg(result.versionNumber).arg(result.versionText));

    /// editor as stated in file format
    metaText.append(QString("<tool origin=\"document\" type=\"editor\" subtype=\"creator\">\n%1</tool>\n").arg(guessTool(result.creatorText)));
    metaText.append(QString("<tool origin=\"document\" type=\"editor\" subtype=\"revisor\">\n%1</tool>\n").arg(guessTool(result.revisorText)));

    /// evaluate editor (a.k.a. creator)
    if (!result.authorInitial.isEmpty())
        headerText.append(QString("<author type=\"first\">%1</author>\n").arg(DocScan::xmlify(result.authorInitial)));
    if (!result.authorLast.isEmpty())
        headerText.append(QString("<author type=\"last\">%1</author>\n").arg(DocScan::xmlify(result.authorLast)));

    /// evaluate title
    if (!result.title.isEmpty())
        headerText.append(QString("<title>%1</title>\n").arg(DocScan::xmlify(result.title)));

    /// evaluate subject
    if (!result.subject.isEmpty())
        headerText.append(QString("<subject>%1</subject>\n").arg(DocScan::xmlify(result.subject)));

    /// evaluate language
    if (!result.language.isEmpty())
        headerText.append(QString("<language origin=\"document\">%1</language>\n").arg(result.language));
    if (result.plainText.length() > 1024)
        headerText.append(QString("<language origin=\"aspell\">%1</language>\n").arg(guessLanguage(result.plainText)));

    /// evaluate dates
    if (result.dateCreation.isValid())
        headerText.append(DocScan::formatDate(result.dateCreation, "creation"));
    if (result.dateModification.isValid())
        headerText.append(DocScan::formatDate(result.dateModification, "modification"));

    /// evaluate number of pages
    if (result.pageCount > 0)
        headerText.append(QString("<num-pages>%1</num-pages>\n").arg(result.pageCount));

    /// evaluate paper size
    if (result.paperSizeHeight > 0 && result.paperSizeWidth > 0)
        headerText.append(evaluatePaperSize(result.paperSizeWidth, result.paperSizeHeight));

    // TODO fonts

    QString bodyText = QString("<body length=\"%1\" />\n").arg(result.plainText.length());

    /// close all tags, merge text
    metaText += QLatin1String("</meta>\n");
    logText.append(metaText);
    headerText += QLatin1String("</header>\n");
    logText.append(headerText);
    logText.append(bodyText);
    logText += QLatin1String("</fileanalysis>\n");

    delete document;

    emit analysisReport(logText);

    m_isAlive = false;
}

/**
 * Some .doc files are only disguised .rtf files, e.g. if the creating editor
 * does not support .doc but wants the user to be able this file in Word with
 * any inconvenience.
 * This trick has been/is been use by Microsoft itself.
 */
bool FileAnalyzerCompoundBinary::isRTFfile(const QString &filename)
{
    const char *rtfHeader = "{\\rtf";
    QFile file(filename);
    if (file.open(QFile::ReadOnly)) {
        QByteArray head = file.read(16);
        file.close();
        return head.contains(rtfHeader);
    }
    return false;
}
