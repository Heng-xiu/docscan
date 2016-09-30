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

#ifndef FILEANALYZERMULTIPLEXER_H
#define FILEANALYZERMULTIPLEXER_H

#include "fileanalyzerabstract.h"
#include "fileanalyzerodf.h"
#include "fileanalyzeropenxml.h"
#include "fileanalyzerpdf.h"
#include "fileanalyzercompoundbinary.h"

/**
 * Automatically redirects a file to be analyzed
 * to the right specialized file analysis object.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerMultiplexer : public FileAnalyzerAbstract
{
    Q_OBJECT
public:
    explicit FileAnalyzerMultiplexer(const QStringList &filters, QObject *parent = 0);

    virtual bool isAlive();

    void setupJhove(const QString &shellscript, const QString &configFile, bool verbose);

public slots:
    virtual void analyzeFile(const QString &filename);

private:
    FileAnalyzerODF m_fileAnalyzerODF;
    FileAnalyzerPDF m_fileAnalyzerPDF;
    FileAnalyzerOpenXML m_fileAnalyzerOpenXML;
    FileAnalyzerCompoundBinary m_fileAnalyzerCompoundBinary;
    const QStringList &m_filters;

    void uncompressAnalyzefile(const QString &filename, const QString &extension, const QString &uncompressTool);
};

#endif // FILEANALYZERMULTIPLEXER_H
