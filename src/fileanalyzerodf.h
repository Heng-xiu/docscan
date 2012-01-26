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

#ifndef FILEANALYZERODF_H
#define FILEANALYZERODF_H

#include <QStringList>
#include <QDate>

#include "fileanalyzerabstract.h"

class QIODevice;

/**
 * Analyzing code for XML-based OpenDocument Text documents.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerODF : public FileAnalyzerAbstract
{
    Q_OBJECT
public:
    explicit FileAnalyzerODF(QObject *parent = 0);

    virtual bool isAlive();

public slots:
    virtual void analyzeFile(const QString &filename);

private:
    enum PageCountOrigin {pcoDocument = 1, pcoOwnCount = 2};
    typedef struct {
        QStringList documentVersionNumbers;
        QString toolGenerator;
        QString authorInitial, authorLast;
        QString title, subject;
        QString language;
        QDate dateCreation, dateModification, datePrint;
        int pageCount;
        PageCountOrigin pageCountOrigin;
        QString plainText;
        int paperSizeWidth, paperSizeHeight;
    } ResultContainer;

    class ODFContentFileHandler;
    class ODFMetaFileHandler;
    class ODFStylesFileHandler;

    bool m_isAlive;

    void analyzeMetaXML(QIODevice &device, ResultContainer &result);
    void analyzeStylesXML(QIODevice &device, ResultContainer &result);
    void text(QIODevice &contentFile, ResultContainer &result);
};

#endif // FILEANALYZERODF_H
