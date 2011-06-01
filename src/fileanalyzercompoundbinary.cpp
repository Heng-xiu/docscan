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

#include <olestream.h>
#include <word_helper.h>
#include <word97_helper.h>
#include <word97_generated.h>

#include "fileanalyzercompoundbinary.h"
#include "general.h"

using namespace wvWare;

FileAnalyzerCompoundBinary::FileAnalyzerCompoundBinary(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
    // nothing
}

bool FileAnalyzerCompoundBinary::isAlive()
{
    return false;
}

void FileAnalyzerCompoundBinary::analyzeFile(const QString &filename)
{
    /// perform various file checks before starting the analysis
    OLEStorage storage(std::string(filename.toUtf8().constData()));
    if (!storage.open(OLEStorage::ReadOnly)) {
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"OLEStorage cannot be opened\" filename=\"%1\" />\n").arg(filename));
        return;
    }

    OLEStreamReader *document = storage.createStreamReader("WordDocument");
    if (document == NULL || !document->isValid()) {
        if (document != NULL)  delete document;
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"Not a valid Word document\" filename=\"%1\" />\n").arg(filename));
        return;
    }

    /// get the FIB (File information block) which contains a lot of interesting information
    Word97::FIB fib(document, true);

    OLEStreamReader *table = storage.createStreamReader(fib.fWhichTblStm ? "1Table" : "0Table");
    if (table == NULL || !table->isValid()) {
        if (document != NULL)  delete document;
        if (table != NULL)  delete table;
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"Cannot read table\" filename=\"%1\" />\n").arg(filename));
        return;
    }

    /// determine mimetype and write file analysis header
    QString mimetype = "application/octet-stream";
    if (filename.endsWith(".doc"))
        mimetype = "application/msword";
    QString logText = QString("<fileanalysis mimetype=\"%1\" filename=\"%2\">\n").arg(mimetype).arg(DocScan::xmlify(filename));

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

    /// read meta information from table
    table->seek(fib.fcSttbfAssoc);
    STTBF sttbf(fib.lid, table);
    int i = 0;
    for (UString s = sttbf.firstString(); !s.isNull(); s = sttbf.nextString(), ++i) {
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

    /// dump data on dates
    logText += "<unknown>" + QString::number((fib.lProductCreated & 0xE0000000) >> 29) + "|" + QString::number((fib.lProductCreated & 0x1FF00000) >> 20) + "|" + QString::number((fib.lProductCreated & 0x000F0000) >> 16) + "|" + QString::number((fib.lProductCreated & 0xF800) >> 11) + "|" + QString::number((fib.lProductCreated & 0x07C0) >> 6) + "|" + QString::number(fib.lProductCreated & 0x003F) + "|" + "</unknown>\n";
    logText += "<unknown>" + QString::number((fib.lProductRevised & 0xE0000000) >> 29) + "|" + QString::number((fib.lProductRevised & 0x1FF00000) >> 20) + "|" + QString::number((fib.lProductRevised & 0x000F0000) >> 16) + "|" + QString::number((fib.lProductRevised & 0xF800) >> 11) + "|" + QString::number((fib.lProductRevised & 0x07C0) >> 6) + "|" + QString::number(fib.lProductRevised & 0x003F) + "|" + "</unknown>\n";

    delete document;
    delete table;

    logText += "</fileanalysis>";
    emit analysisReport(logText);
}
