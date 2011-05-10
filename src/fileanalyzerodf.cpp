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

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include <QDomDocument>
#include <QDebug>

#include "fileanalyzerodf.h"

FileAnalyzerODF::FileAnalyzerODF(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
}

void FileAnalyzerODF::analyzeFile(const QString &filename)
{
    qDebug() << "analyzing file" << filename;
    QuaZip zipFile(filename);

    if (zipFile.open(QuaZip::mdUnzip)) {
        if (zipFile.setCurrentFile("meta.xml", QuaZip::csInsensitive)) {
            QuaZipFile metaXML(&zipFile, parent());
            QDomDocument metaDocument;
            metaDocument.setContent(&metaXML);
            analyzeMetaXML(metaDocument);
        }
    }
}

void FileAnalyzerODF::analyzeMetaXML(QDomDocument &metaXML)
{
    QDomElement rootNode = metaXML.documentElement();
    qDebug() << "root node=" << rootNode.attributes().namedItem("office:version").nodeValue();
    QDomNode officeMetaNode = metaXML.elementsByTagName("office:meta").item(0);
    QDomElement dcCreatorNode = officeMetaNode.firstChildElement("dc:creator");
    qDebug() << "dcCreatorNode=" << dcCreatorNode.nodeValue();
}