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

#include <poppler/qt4/poppler-qt4.h>

#include <QDebug>

#include "fileanalyzerpdf.h"

FileAnalyzerPDF::FileAnalyzerPDF(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
    // nothing
}

void FileAnalyzerPDF::analyzeFile(const QString &filename)
{
    qDebug() << "analyzing file" << filename;
    Poppler::Document *doc = Poppler::Document::load(filename);

    if (doc != NULL) {
        int majorVersion = 0, minorVersion = 0;
        doc->getPdfVersion(&majorVersion, &minorVersion);

        QDateTime creationDate =  doc->date("CreationDate");
        QDateTime modificationDate =  doc->date("ModDate");

        QString title = doc->info("Title");
        QString subject = doc->info("Subject");
        QString author = doc->info("Author");
        QString keywords = doc->info("Keywords");
        QString creator = doc->info("Creator");
        QString producer = doc->info("Producer");

        qDebug() << title << subject << author << keywords << creator << producer;

        delete doc;
    }

}

bool FileAnalyzerPDF::isAlive()
{
    return false;
}
