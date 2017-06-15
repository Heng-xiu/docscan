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


    Copyright (2017) Thomas Fischer <thomas.fischer@his.se>, senior
    lecturer at University of Skövde, as part of the LIM-IT project.

 */

#ifndef FILEANALYZERPDF_H
#define FILEANALYZERPDF_H

#include <QObject>

#include <poppler-qt5.h>

#include "fileanalyzerabstract.h"
#include "jhovewrapper.h"

/**
 * Analyzing code for Portable Document File documents.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerPDF : public FileAnalyzerAbstract, public JHoveWrapper
{
    Q_OBJECT
public:
    explicit FileAnalyzerPDF(QObject *parent = nullptr);

    virtual bool isAlive();

    void setupJhove(const QString &shellscript);
    void setupVeraPDF(const QString &cliTool);
    void setupPdfBoXValidator(const QString &pdfboxValidatorJavaClass);
    void setupCallasPdfAPilotCLI(const QString &callasPdfAPilotCLI);

public slots:
    virtual void analyzeFile(const QString &filename);

private:
    struct ExtendedFontInfo {
        QString name, typeName, fileName;
        bool isEmbedded, isSubset;
        int firstPageNumber, lastPageNumber;

        explicit ExtendedFontInfo()
            : firstPageNumber(INT_MAX), lastPageNumber(INT_MIN) {

        }

        explicit ExtendedFontInfo(const Poppler::FontInfo &fi, const int pageNumber)
            : name(fi.name()), typeName(fi.typeName()), fileName(fi.file()), isEmbedded(fi.isEmbedded()), isSubset(fi.isSubset()),
              firstPageNumber(pageNumber), lastPageNumber(pageNumber)
        {
            /// nothing
        }

        void recordOccurrence(int pageNumber) {
            if (pageNumber < firstPageNumber) firstPageNumber = pageNumber;
            if (pageNumber > lastPageNumber) lastPageNumber = pageNumber;
        }

        bool isValid() const {
            return lastPageNumber > INT_MIN && firstPageNumber < INT_MAX && firstPageNumber <= lastPageNumber;
        }
    };

    bool m_isAlive;
    QString m_veraPDFcliTool;
    QString m_pdfboxValidatorJavaClass;
    QString m_callasPdfAPilotCLI;

    bool popplerAnalysis(const QString &filename, QString &logText, QString &metaText);
};

#endif // FILEANALYZERPDF_H
