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

#ifndef FILEANALYZEROPENXML_H
#define FILEANALYZEROPENXML_H

#include <QStringList>

#include "fileanalyzerabstract.h"

class QuaZip;
class QIODevice;

/**
 * Analyzing code for XML-based Microsoft Word documents.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerOpenXML : public FileAnalyzerAbstract
{
    Q_OBJECT
public:
    explicit FileAnalyzerOpenXML(QObject *parent = nullptr);

    virtual bool isAlive();

public slots:
    virtual void analyzeFile(const QString &filename);

private:
    typedef struct {
        QString toolGenerator;
        QString formatVersion;
        QString authorInitial, authorLast;
        QString title, subject;
        QString languageAspell, languageDocument;
        QString dateCreation, dateModification;
        int pageCount;
        QString plainText;
        int characterCount;
        int paperSizeWidth, paperSizeHeight;
    } ResultContainer;

    class OpenXMLDocumentHandler;
    class OpenXMLCoreHandler;
    class OpenXMLAppHandler;
    class OpenXMLSettingsHandler;
    class OpenXMLSlideHandler;

    bool m_isAlive;

    bool processWordFile(QuaZip &zipFile, ResultContainer &result);
    bool processCore(QuaZip &zipFile, ResultContainer &result);
    bool processApp(QuaZip &zipFile, ResultContainer &result);
    bool processSettings(QuaZip &zipFile, ResultContainer &result);
    bool processSlides(QuaZip &zipFile, ResultContainer &result);
    void text(QIODevice &device, ResultContainer &result);
};

#endif // FILEANALYZEROPENXML_H
