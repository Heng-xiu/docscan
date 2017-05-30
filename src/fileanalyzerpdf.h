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

#include "fileanalyzerabstract.h"

namespace Poppler
{
class Document;
}

/**
 * Analyzing code for Portable Document File documents.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerPDF : public FileAnalyzerAbstract
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
    bool m_isAlive;
    QString m_jhoveShellscript;
    QString m_veraPDFcliTool;
    QString m_pdfboxValidatorJavaClass;
    QString m_callasPdfAPilotCLI;

    bool popplerAnalysis(const QString &filename, QString &logText, QString &metaText);
};

#endif // FILEANALYZERPDF_H
