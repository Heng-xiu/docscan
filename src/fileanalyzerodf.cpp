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

#include <QIODevice>
#include <QDebug>
#include <QStringList>
#include <QXmlDefaultHandler>
#include <QXmlSimpleReader>
#include <QStack>

#include "fileanalyzerodf.h"
#include "watchdog.h"
#include "general.h"

class ODFMetaFileHandler: public  QXmlDefaultHandler
{
private:
    QString &m_logText;
    QStack<QString> m_nodeName;
public:
    ODFMetaFileHandler(QString &logText)
        : QXmlDefaultHandler(), m_logText(logText) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(localName);
        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        QString nodeName = m_nodeName.top().toLower();

        QDate date = QDate::fromString(text.left(10), "yyyy-MM-dd");
        if (nodeName == "title" || nodeName == "subject")
            m_logText += QString("<%1>%2</%1>\n").arg(nodeName).arg(DocScan::xmlify(text));
        else if (nodeName == "generator")
            m_logText += QString("<generator>%1</generator>\n").arg(DocScan::xmlify(text));
        else if (nodeName == "creator" || nodeName == "initial-creator")
            m_logText += QString("<meta name=\"%1\">%2</meta>\n").arg(nodeName.replace("creator", "author")).arg(DocScan::xmlify(text));
        else if (nodeName == "keyword")
            m_logText += QString("<meta name=\"%1s\">%2</meta>\n").arg(nodeName).arg(DocScan::xmlify(text));
        else if (nodeName == "creation-date" && date.isValid())
            m_logText += QString("<date base=\"creation\" year=\"%1\" month=\"%2\" day=\"%3\">%4</date>\n").arg(date.year()).arg(date.month()).arg(date.day()).arg(date.toString(Qt::ISODate));
        else if (nodeName == "date" && date.isValid())
            m_logText += QString("<date base=\"modification\" year=\"%1\" month=\"%2\" day=\"%3\">%4</date>\n").arg(date.year()).arg(date.month()).arg(date.day()).arg(date.toString(Qt::ISODate));
        else if (nodeName == "print-date" && date.isValid())
            m_logText += QString("<date base=\"print\" year=\"%1\" month=\"%2\" day=\"%3\">%4</date>\n").arg(date.year()).arg(date.month()).arg(date.day()).arg(date.toString(Qt::ISODate));

        return QXmlDefaultHandler::characters(text);
    }
};

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

        QString logText = QString("<fileanalysis mimetype=\"%1\" filename=\"%2\">\n").arg(mimetype).arg(DocScan::xmlify(filename));

        if (zipFile.setCurrentFile("meta.xml", QuaZip::csInsensitive)) {
            QuaZipFile metaXML(&zipFile, parent());
            analyzeMetaXML(metaXML, logText);
        } else {
            emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-meta\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        logText += "</fileanalysis>\n";

        emit analysisReport(logText);
    } else
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-fileformat\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
}

void FileAnalyzerODF::analyzeMetaXML(QIODevice &device, QString &logText)
{
    ODFMetaFileHandler handler(logText);
    QXmlInputSource source(&device);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(&source);
}
