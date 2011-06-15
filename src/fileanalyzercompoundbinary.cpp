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


class DocScanTextHandler: public wvWare::TextHandler
{
private:
    QString lidToLangCode(int lid) {
        switch (lid) {
        case 0x0c09:
        case 0x0809:
        case 0x0409: return QLatin1String("en");
        case 0x807:
        case 0x407: return QLatin1String("de");
        case 0x41d: return QLatin1String("sv");
        default: {
            QString language;
            language.setNum(lid, 16);
            return "0x" + language;
        }
        }
    }

public:
    QString wholeText;
    QString language;

    DocScanTextHandler()
        : wvWare::TextHandler() {
        wholeText = "";
        language = "";
    }

    void runOfText(const wvWare::UString &text, wvWare::SharedPtr<const wvWare::Word97::CHP> chp) {
        QString qString = string(text);
        wholeText += qString;
        if (language.isEmpty())
            language = lidToLangCode(chp->lidDefault);
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

void FileAnalyzerCompoundBinary::analyzeFiB(wvWare::Word97::FIB &fib, QString &logText)
{

    /// try to guess operating system based on header information
    // TODO: What value is set by non-Microsoft editors on e.g. Linux?
    QString opsys;
    switch (fib.envr) {
    case 0: opsys = "windows"; break;
    case 1: opsys = "unix|macos"; break;
    default: opsys = QString("unknown=%1").arg(fib.envr);
    }

    /// determine version format of this file
    // FIXME this number guessing doesn't see correct
    int versionNumber = -1;
    QString versionText;
    switch (fib.nFib) {
    case 0x101:
    case 0x0065:
        versionNumber = 6;
        versionText = "Word 6.0";
        break;
    case 0x104:
    case 0x0068:
        versionNumber = 7;
        versionText = "Word 95";
        break;
    case 0x105:
    case 0x00c1:
        versionNumber = 8;
        versionText = "Word 97";
        break;
    case 0x00d9:
        versionNumber = 9;
        versionText = "Word 2000";
        break;
    default:
        versionNumber = 0;
        versionText = QString::number(fib.nFib, 16);
    }
    if (versionNumber >= 0) {
        logText += QString("<fileformat-version major=\"%1\" minor=\"0\">%2</fileformat-version>\n").arg(versionNumber).arg(versionText);
    }

    /// determine used editor
    QString editorString;
    switch (fib.wMagicCreated) {
    case 0x6a62:
        editorString = "Microsoft Word 97";
        break;
    case 0x626a:
        editorString = "Microsoft Word 98/Mac";
        break;
    case 0x6143:
        editorString = "unnamed (0x6143)";
        break;
    case 0xa5dc:
        editorString = "Microsoft Word 6.0/7.0";
        break;
    case 0xa5ec:
        editorString = "Microsoft Word 8.0";
        break;
    default:
        editorString = QString::number(fib.wMagicCreated, 16);
    }
    logText += QString("<generator license=\"proprietary\" opsys=\"windows\" type=\"editor\" origin=\"document\">%1</generator>\n").arg(editorString);
}

void FileAnalyzerCompoundBinary::analyzeTable(wvWare::OLEStorage &storage, wvWare::Word97::FIB &fib, QString &logText)
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
            logText += QString("<title>%1</title>\n").arg(DocScan::xmlify(s.ascii()));
            break;
        case 3:
            logText += QString("<subject>%1</subject>\n").arg(DocScan::xmlify(s.ascii()));
            break;
        case 4:
            logText += QString("<keywords>%1</keywords>\n").arg(DocScan::xmlify(s.ascii()));
            break;
        case 6:
            logText += QString("<meta name=\"initial-creator\">%1</meta>\n").arg(DocScan::xmlify(s.ascii()));
            break;
        case 7:
            logText += QString("<meta name=\"creator\">%1</meta>\n").arg(DocScan::xmlify(s.ascii()));
            break;
        }
    }

    delete table;
}

void FileAnalyzerCompoundBinary::analyzeWithParser(std::string &filename, QString &logText)
{
    wvWare::SharedPtr<wvWare::Parser> parser(wvWare::ParserFactory::createParser(filename));
    if (parser != NULL) {
        if (parser->isOk()) {
            DocScanTextHandler *textHandler = new DocScanTextHandler();
            parser->setTextHandler(textHandler);

            if (parser->parse()) {
                const wvWare::Word97::DOP dop = parser->dop();

                QDate creationDate(1900 + dop.dttmCreated.yr, dop.dttmCreated.mon, dop.dttmCreated.dom);
                logText += DocScan::formatDate(creationDate, "creation");

                QDate modificationDate(1900 + dop.dttmRevised.yr, dop.dttmRevised.mon, dop.dttmRevised.dom);
                logText += DocScan::formatDate(modificationDate, "modification");

                QString language;
                if (textHandler->wholeText.length() > 1024 && !(language = guessLanguage(textHandler->wholeText)).isEmpty())
                    logText += "<meta name=\"language\" origin=\"aspell\">" + language + "</meta>\n";
                if (!textHandler->language.isEmpty())
                    logText += "<meta name=\"language\" origin=\"document\">" + textHandler->language + "</meta>\n";
            }

            delete textHandler;
        }
    }

}

void FileAnalyzerCompoundBinary::analyzeFile(const QString &filename)
{
    m_isAlive = true;

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
    QString logText = QString("<fileanalysis mimetype=\"%1\" filename=\"%2\">\n").arg(mimetype).arg(DocScan::xmlify(filename));

    /// get the FIB (File information block) which contains a lot of interesting information
    wvWare::Word97::FIB fib(document, true);
    analyzeFiB(fib, logText);

    /// read meta information from table
    analyzeTable(storage, fib, logText);

    analyzeWithParser(cppFilename, logText);

    delete document;

    logText += "</fileanalysis>\n";
    emit analysisReport(logText);

    m_isAlive = false;
}
