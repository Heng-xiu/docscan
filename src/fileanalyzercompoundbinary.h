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

#ifndef FILEANALYZERCOMPOUNDBINARY_H
#define FILEANALYZERCOMPOUNDBINARY_H

#include <QDate>

#include <word_helper.h>
#include <word97_generated.h>

#include "fileanalyzerabstract.h"

class FileAnalyzerCompoundBinary : public FileAnalyzerAbstract
{
    Q_OBJECT
public:
    explicit FileAnalyzerCompoundBinary(QObject *parent = 0);

    virtual bool isAlive();

    static bool isRTFfile(const QString &filename);
    static QString langCodeToISOCode(int lid);

public slots:
    virtual void analyzeFile(const QString &filename);

private:
    typedef struct {
        int versionNumber;
        QString versionText;
        QString creatorText, revisorText;
        QString editorText;
        QString opSys;
        QString title, subject, keywords;
        QString authorInitial, authorLast;
        QString plainText, language;
        QDate dateCreation, dateModification;
        int pageCount, charCount;
        int paperSizeWidth, paperSizeHeight;
    } ResultContainer;

    bool m_isAlive;

    void analyzeFiB(wvWare::Word97::FIB &fib, ResultContainer &result);
    void analyzeTable(wvWare::OLEStorage &storage, wvWare::Word97::FIB &fib, ResultContainer &result);
    void analyzeWithParser(std::string &filename, ResultContainer &result);

    bool getVersion(unsigned short nFib, int &versionNumber, QString &versionText);
    bool getEditor(unsigned short wMagic, QString &editorText);

    class DocScanTextHandler;
};

#endif // FILEANALYZERCOMPOUNDBINARY_H
