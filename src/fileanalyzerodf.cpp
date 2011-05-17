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
#include <QStringList>

#include "fileanalyzerodf.h"
#include "watchdog.h"

FileAnalyzerODF::FileAnalyzerODF(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
}

bool FileAnalyzerODF::isAlive()
{
    return false;
}

void FileAnalyzerODF::analyzeFile(const QString &filename)
{
    qDebug() << "analyzing file" << filename;
    QuaZip zipFile(filename);

    if (zipFile.open(QuaZip::mdUnzip)) {
        QString mimetype = "application/octet-stream";

        if (zipFile.setCurrentFile("mimetype", QuaZip::csInsensitive)) {
            QuaZipFile mimetypeFile(&zipFile, parent());
            if (mimetypeFile.open(QIODevice::ReadOnly)) {
                QTextStream ts(&mimetypeFile);
                mimetype = ts.readLine();
            }
        }

        QString logText = QString("<fileanalysis mimetype=\"%1\" filename=\"%2\">\n").arg(mimetype).arg(filename);

        if (zipFile.setCurrentFile("meta.xml", QuaZip::csInsensitive)) {
            QuaZipFile metaXML(&zipFile, parent());
            QDomDocument metaDocument;
            metaDocument.setContent(&metaXML);
            analyzeMetaXML(metaDocument, logText);
        }

        logText += "<fileanalysis/>\n";

        emit analysisReport(logText);
    }
}

void FileAnalyzerODF::analyzeMetaXML(QDomDocument &metaXML, QString &logText)
{
    QDomElement rootNode = metaXML.documentElement();

    QRegExp versionRegExp("(\\d+)\\.(\\d+)");
    if (versionRegExp.indexIn(rootNode.attributes().namedItem("office:version").nodeValue()))
        logText += QString("<version major=\"%1\" minor=\"%2\" />\n").arg(versionRegExp.cap(1)).arg(versionRegExp.cap(2));

    /*
    QDomNode officeMetaNode = rootNode.firstChildElement("office:meta");
    qDebug() << "officeMetaNode=" << officeMetaNode.nodeValue() << officeMetaNode.localName() << officeMetaNode.nodeName();
    QDomElement dcCreatorNode = officeMetaNode.firstChildElement("dc:creator");
    qDebug() << "dcCreatorNode=" << dcCreatorNode.childNodes().item(0).toText().data() << dcCreatorNode.nodeName();
    qDebug() << "dcCreatorNode=" << getValue(QStringList() << "office:meta" << "dc:creator", rootNode);
    */
}

QString FileAnalyzerODF::getValue(const QStringList &path, const QDomElement &root)
{
    QDomNode cur = root;
    QString text;
    foreach(QString pathElement, path) {
        text = cur.toText().data();
        cur = cur.firstChildElement(pathElement);
    }
    return text;
}
