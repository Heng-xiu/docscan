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
        case 0x0c09:
        case 0x0809:
        case 0x0409: return QLatin1String("en");
        case 0x807:
        case 0x407: return QLatin1String("de");
        case 0x41c: return QLatin1String("en"); ///< ?
        case 0x41d: return QLatin1String("sv");
        case 0x410: return QLatin1String("it"); ///< ?
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

    /// determine version format of this file
    // FIXME this number guessing doesn't see correct
    switch (fib.nFib) {
    case 0x101:
    case 0x0065:
        result.versionNumber = 6;
        result.versionText = QLatin1String("Word 6.0");
        break;
    case 0x104:
    case 0x0068:
        result.versionNumber = 7;
        result.versionText = QLatin1String("Word 95");
        break;
    case 0x105:
    case 0x00c1:
        result.versionNumber = 8;
        result.versionText = QLatin1String("Word 97");
        break;
    case 0x00d9:
        result.versionNumber = 9;
        result.versionText = QLatin1String("Word 2000");
        break;
    default:
        result.versionNumber = 0;
        result.versionText = QString::number(fib.nFib, 16);
    }

    /// determine used editor
    switch (fib.wMagicCreated) {
    case 0x6a62:
        result.editorText = QLatin1String("Microsoft Word 97");
        break;
    case 0x626a:
        result.editorText = QLatin1String("Microsoft Word 98/Mac");
        break;
    case 0x6143:
        result.editorText = QLatin1String("unnamed (0x6143)");
        break;
    case 0xa5dc:
        result.editorText = QLatin1String("Microsoft Word 6.0/7.0");
        break;
    case 0xa5ec:
        result.editorText = QLatin1String("Microsoft Word 8.0");
        break;
    default:
        result.editorText = QString::number(fib.wMagicCreated, 16);
    }
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
                result.paperWidth = 0;
                result.paperHeight = 0;
            }

            delete textHandler;
        }
    }

}

void FileAnalyzerCompoundBinary::analyzeFile(const QString &filename)
{
    m_isAlive = true;
    ResultContainer result;

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
    metaText.append(QString("<fileformat>\n<mimetype>%3</mimetype>\n<version major=\"%1\" minor=\"0\">%2</version>\n</fileformat>").arg(result.versionNumber).arg(result.versionText).arg(mimetype));

    /// editor as stated in file format
    QString guess = guessTool(result.editorText);
    metaText.append(QString("<tool origin=\"document\" type=\"editor\">\n%1</tool>\n").arg(guess));

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

    // TODO paper size

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
