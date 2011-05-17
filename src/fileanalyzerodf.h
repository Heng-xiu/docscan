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

#include "fileanalyzerabstract.h"

class QDomElement;
class QDomDocument;

class FileAnalyzerODF : public FileAnalyzerAbstract
{
    Q_OBJECT
public:
    explicit FileAnalyzerODF(QObject *parent = 0);

    virtual bool isAlive();

public slots:
    virtual void analyzeFile(const QString &filename);

private:
    void analyzeMetaXML(QDomDocument &metaXML, QString &logText);
    QString getValue(const QStringList &path, const QDomElement &root);
};

#endif // FILEANALYZERODF_H
