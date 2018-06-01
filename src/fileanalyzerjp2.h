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

#ifndef FILEANALYZERJP2_H
#define FILEANALYZERJP2_H

#include <QObject>

#include "fileanalyzerabstract.h"
#include "jhovewrapper.h"

/**
 * Analyzing code for JPEG2000 data
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerJP2 : public FileAnalyzerAbstract, public JHoveWrapper
{
    Q_OBJECT
public:
    explicit FileAnalyzerJP2(QObject *parent = nullptr);

    virtual bool isAlive() override;

    void setupJhove(const QString &shellscript);

public slots:
    virtual void analyzeFile(const QString &filename) override;
};

#endif // FILEANALYZERJP2_H
