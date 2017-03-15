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

#include <quazip.h>
#include <quazipfile.h>

#include <QIODevice>
#include <QDebug>
#include <QStringList>
#include <QXmlDefaultHandler>
#include <QXmlSimpleReader>
#include <QStack>
#include <QFileInfo>

#include "fileanalyzerodf.h"
#include "watchdog.h"
#include "general.h"



class FileAnalyzerODF::ODFContentFileHandler: public QXmlDefaultHandler
{
private:
    ResultContainer &result;
    QStack<QString> m_nodeName;
    bool m_insideText;
    int m_pageCount;

public:
    ODFContentFileHandler(FileAnalyzerODF *, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), result(resultContainer), m_insideText(false), m_pageCount(0) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);
        m_insideText |= (qName == "office:text" || qName == "office:presentation" || qName == "office:spreadsheet") && m_nodeName.count() >= 2 && m_nodeName.at(m_nodeName.count() - 2) == "office:body";
        if (qName == "draw:page" || qName == "table:table") ++m_pageCount;
        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        if (qName == "office:text" || qName == "office:presentation" ||  qName == "office:spreadsheet" || qName == "office:body") m_insideText = false;
        if (qName == "office:document-content" && m_pageCount > 0 && result.pageCountOrigin == 0) {
            result.pageCountOrigin = pcoOwnCount;
            result.pageCount = m_pageCount;
        }

        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        if (m_insideText) result.plainText += text;
        return QXmlDefaultHandler::characters(text);
    }
};

class FileAnalyzerODF::ODFStylesFileHandler: public QXmlDefaultHandler
{
private:
    ResultContainer &result;
    QStack<QString> m_nodeName;
    QString stylepagelayoutname;
    QMap<QString, QString> stylepagelayoutproperties;
    QString m_lastStyleName;

public:
    ODFStylesFileHandler(FileAnalyzerODF *, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), result(resultContainer), stylepagelayoutname(QString::null) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);

        if (m_nodeName.count() >= 2 && qName == "style:master-page" && atts.value("style:name") == "Standard" && m_nodeName.at(m_nodeName.count() - 2) == "office:master-styles")
            stylepagelayoutname = atts.value("style:page-layout-name");
        else if (qName == "style:page-layout")
            m_lastStyleName = atts.value("style:name");
        else if (m_nodeName.count() >= 2 && qName == "style:page-layout-properties" && m_nodeName.at(m_nodeName.count() - 2) == "style:page-layout") {
            stylepagelayoutproperties.insert(m_lastStyleName, atts.value("fo:page-width") + "|" + atts.value("fo:page-height"));
        }

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();

        if (qName == "office:document-styles") {
            /// end of file

            QString documentProperties;
            if (!stylepagelayoutname.isEmpty() && stylepagelayoutproperties.contains(stylepagelayoutname)) {
                QStringList sizes = stylepagelayoutproperties[stylepagelayoutname].split("|");
                if (sizes[0].endsWith("in")) {
                    bool ok = false;
                    result.paperSizeWidth = sizes[0].left(sizes[0].length() - 2).toDouble(&ok) * 25.4;
                    result.paperSizeHeight = sizes[1].left(sizes[1].length() - 2).toDouble(&ok) * 25.4;
                } else if (sizes[0].endsWith("cm")) {
                    bool ok = false;
                    result.paperSizeWidth = sizes[0].left(sizes[0].length() - 2).toDouble(&ok) * 10;
                    result.paperSizeHeight = sizes[1].left(sizes[1].length() - 2).toDouble(&ok) * 10;
                } else if (sizes[0].endsWith("mm")) {
                    bool ok = false;
                    result.paperSizeWidth = sizes[0].left(sizes[0].length() - 2).toDouble(&ok);
                    result.paperSizeHeight = sizes[1].left(sizes[1].length() - 2).toDouble(&ok);
                }
            }
        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        QString nodeName = m_nodeName.top().toLower();
// TODO
        return QXmlDefaultHandler::characters(text);
    }


};

class FileAnalyzerODF::ODFMetaFileHandler: public QXmlDefaultHandler
{
private:
    FileAnalyzerODF *p;
    ResultContainer &result;
    QStack<QString> nodeNameStack;


public:
    ODFMetaFileHandler(FileAnalyzerODF *parent, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), p(parent), result(resultContainer) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        nodeNameStack.push(qName);

        if (result.pageCountOrigin == 0 && qName == "meta:document-statistic" && !atts.value("meta:page-count").isEmpty()) {
            bool ok = false;
            result.pageCount = atts.value("meta:page-count").toInt(&ok);
            if (!ok)
                result.pageCount = 0;
            else
                result.pageCountOrigin = pcoDocument;
        } else if (qName == "office:document-meta" && !atts.value("office:version").isEmpty()) {
            result.documentVersionNumbers = atts.value("office:version").split(".");
        }

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        nodeNameStack.pop();
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        QString nodeName = nodeNameStack.top().toLower();

        if (nodeName == "meta:generator") {
            QString guess = p->guessTool(text);
            if (!guess.isEmpty())
                result.toolGenerator = QString("<tool type=\"generator\">\n%1</tool>\n").arg(guess);
        } else if (nodeName == "dc:initial-creator") {
            result.authorInitial = QString("<author type=\"first\">%1</author>\n").arg(DocScan::xmlify(text));
        } else if (nodeName == "dc:creator") {
            result.authorLast = QString("<author type=\"last\">%1</author>\n").arg(DocScan::xmlify(text));
        } else if (nodeName == "dc:title") {
            result.title = QString("<title>%1</title>\n").arg(DocScan::xmlify(text));
        } else if (nodeName == "dc:subject") {
            result.subject = QString("<subject>%1</subject>\n").arg(DocScan::xmlify(text));
        } else if (nodeName == "dc:language") {
            result.language = QString("<language origin=\"document\">%1</language>\n").arg(text);
        } else {
            QDate date = QDate::fromString(text.left(10), "yyyy-MM-dd");
            if (nodeName == "meta:creation-date" && date.isValid()) {
                result.dateCreation = date;
            } else if (nodeName == "dc:date" && date.isValid()) {
                result.dateModification = date;
            } else if (nodeName == "meta:print-date" && date.isValid()) {
                result.datePrint = date;
            }
        }

        return QXmlDefaultHandler::characters(text);
    }
};

FileAnalyzerODF::FileAnalyzerODF(QObject *parent)
    : FileAnalyzerAbstract(parent), m_isAlive(false)
{
}

bool FileAnalyzerODF::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerODF::analyzeFile(const QString &filename)
{
    m_isAlive = true;
    QuaZip zipFile(filename);

    if (zipFile.open(QuaZip::mdUnzip)) {
        ResultContainer result;
        result.pageCount = 0;
        result.paperSizeWidth = 0;
        result.paperSizeHeight = 0;
        result.pageCountOrigin = (PageCountOrigin)0;

        /// evaluate meta.xml file
        if (zipFile.setCurrentFile("meta.xml", QuaZip::csInsensitive)) {
            QuaZipFile metaXML(&zipFile, parent());
            analyzeMetaXML(metaXML, result);
        } else {
            emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-meta\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        /// evaluate styles.xml file
        if (zipFile.setCurrentFile("styles.xml", QuaZip::csInsensitive)) {
            QuaZipFile stylesXML(&zipFile, parent());
            analyzeStylesXML(stylesXML, result);
        } else {
            emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-styles\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        /// evaluate content.xml file
        if (zipFile.setCurrentFile("content.xml", QuaZip::csInsensitive)) {
            QuaZipFile contentXML(&zipFile, parent());
            text(contentXML, result);
        } else {
            emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-content\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        /// determine mime type
        QString mimetype = "application/octet-stream";
        if (zipFile.setCurrentFile("mimetype", QuaZip::csInsensitive)) {
            QuaZipFile mimetypeFile(&zipFile, parent());
            if (mimetypeFile.open(QIODevice::ReadOnly)) {
                QTextStream ts(&mimetypeFile);
                mimetype = ts.readLine();
                mimetypeFile.close();
            }
        }

        QString logText = QString("<fileanalysis filename=\"%1\" status=\"ok\">\n").arg(DocScan::xmlify(filename));
        QString metaText = QStringLiteral("<meta>\n");
        QString headerText = QStringLiteral("<header>\n");

        /// file format including mime type and file format version
        const QString majorVersion = result.documentVersionNumbers.count() >= 1 ? result.documentVersionNumbers[0] : QString::null;
        const QString minorVersion = result.documentVersionNumbers.count() >= 2 ? result.documentVersionNumbers[1] : QStringLiteral("0");
        metaText.append(QString("<fileformat>\n<mimetype>%1</mimetype>\n").arg(mimetype));
        if (result.documentVersionNumbers.count() > 0 && !majorVersion.isNull())
            metaText.append(QString("<version major=\"%1\" minor=\"%2\">%1.%2</version>\n").arg(majorVersion).arg(minorVersion));
        metaText.append(QStringLiteral("</fileformat>"));

        /// file information including size
        QFileInfo fi = QFileInfo(filename);
        metaText.append(QString("<file size=\"%1\" />").arg(fi.size()));

        /// evaluate used tool
        if (!result.toolGenerator.isEmpty())
            metaText.append(result.toolGenerator);

        /// evaluate editor (a.k.a. creator)
        if (!result.authorInitial.isEmpty())
            headerText.append(result.authorInitial);
        if (!result.authorLast.isEmpty())
            headerText.append(result.authorLast);

        /// evaluate title
        if (!result.title.isEmpty())
            headerText.append(result.title);

        /// evaluate subject
        if (!result.subject.isEmpty())
            headerText.append(result.subject);

        /// evaluate language
        if (!result.language.isEmpty())
            headerText.append(result.language);
        /* Disabling aspell, computationally expensive
        if (result.plainText.length() > 1024)
            headerText.append(QString("<language origin=\"aspell\">%1</language>\n").arg(guessLanguage(result.plainText)));
         */

        /// evaluate paper size
        if (result.paperSizeHeight > 0 && result.paperSizeWidth > 0)
            headerText.append(evaluatePaperSize(result.paperSizeWidth, result.paperSizeHeight));

        /// evaluate number of pages
        if (result.pageCount > 0)
            headerText.append(QString("<num-pages origin=\"%2\">%1</num-pages>\n").arg(result.pageCount).arg(result.pageCountOrigin == pcoDocument ? "document" : "own-count"));

        /// evaluate dates
        if (result.dateCreation.isValid())
            headerText.append(DocScan::formatDate(result.dateCreation, QStringLiteral("creation")));
        if (result.dateModification.isValid())
            headerText.append(DocScan::formatDate(result.dateModification, QStringLiteral("modification")));
        if (result.datePrint.isValid())
            headerText.append(DocScan::formatDate(result.datePrint, QStringLiteral("print")));

        // TODO fonts

        QString bodyText = QString("<body length=\"%1\" />\n").arg(result.plainText.length());

        /// close all tags, merge text
        metaText += QStringLiteral("</meta>\n");
        logText.append(metaText);
        headerText += QStringLiteral("</header>\n");
        logText.append(headerText);
        logText.append(bodyText);
        logText += QStringLiteral("</fileanalysis>\n");

        emit analysisReport(logText);
        zipFile.close();
    } else
        emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));

    m_isAlive = false;
}

void FileAnalyzerODF::analyzeMetaXML(QIODevice &device, ResultContainer &result)
{
    ODFMetaFileHandler handler(this, result);
    QXmlInputSource source(&device);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(&source);
}

void FileAnalyzerODF::analyzeStylesXML(QIODevice &device, ResultContainer &result)
{
    ODFStylesFileHandler handler(this, result);
    QXmlInputSource source(&device);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(&source);
}

void FileAnalyzerODF::text(QIODevice &device, ResultContainer &result)
{
    ODFContentFileHandler handler(this, result);
    QXmlInputSource source(&device);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(&source);
}
