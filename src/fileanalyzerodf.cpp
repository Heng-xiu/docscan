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


class ODFContentFileHandler: public QXmlDefaultHandler
{
private:
    QString &m_text;
    QString &m_logText;
    QStack<QString> m_nodeName;
    bool m_insideText;
    int m_pageCount;

public:
    ODFContentFileHandler(QString &text, QString &logText)
        : QXmlDefaultHandler(), m_text(text), m_logText(logText), m_insideText(false), m_pageCount(0) {
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
        if (qName == "office:document-content" && m_pageCount > 0 && m_logText.indexOf("<statistics type=\"pagecount\"") < 0)
            m_logText += "<statistics type=\"pagecount\" origin=\"counted\">" + QString::number(m_pageCount) + "</statistics>\n";

        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        if (m_insideText && m_text.length() < 16384) m_text += text;
        return QXmlDefaultHandler::characters(text);
    }
};

class ODFStylesFileHandler: public QXmlDefaultHandler
{
private:
    QString &m_logText;
    QStack<QString> m_nodeName;
    QString stylepagelayoutname;
    QMap<QString, QString> stylepagelayoutproperties;
    QString m_lastStyleName;

public:
    ODFStylesFileHandler(QString &logText)
        : QXmlDefaultHandler(), m_logText(logText), stylepagelayoutname(QString::null) {
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
                int mmw = 0, mmh = 0;
                if (sizes[0].endsWith("in")) {
                    bool ok = false;
                    mmw = sizes[0].left(sizes[0].length() - 2).toDouble(&ok) * 25.4;
                    mmh = sizes[1].left(sizes[1].length() - 2).toDouble(&ok) * 25.4;
                } else if (sizes[0].endsWith("cm")) {
                    bool ok = false;
                    mmw = sizes[0].left(sizes[0].length() - 2).toDouble(&ok) * 10;
                    mmh = sizes[1].left(sizes[1].length() - 2).toDouble(&ok) * 10;
                } else if (sizes[0].endsWith("mm")) {
                    bool ok = false;
                    mmw = sizes[0].left(sizes[0].length() - 2).toDouble(&ok);
                    mmh = sizes[1].left(sizes[1].length() - 2).toDouble(&ok);
                }
                documentProperties += FileAnalyzerODF::evaluatePaperSize(mmw, mmh);
            }
            m_logText +=  documentProperties;
        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        QString nodeName = m_nodeName.top().toLower();
// TODO
        return QXmlDefaultHandler::characters(text);
    }


};

class ODFMetaFileHandler: public QXmlDefaultHandler
{
private:
    QString &m_logText;
    QStack<QString> m_nodeName;

    QString interpeteGenerator(const QString &generatorString) {
        QString arguments;

        if (generatorString.indexOf("NeoOffice") >= 0)
            arguments += " opsys=\"unix|macos\"";
        else if (generatorString.indexOf("Win32") >= 0)
            arguments += " opsys=\"windows|win32\"";
        else if (generatorString.indexOf("MicrosoftOffice") >= 0)
            arguments += " opsys=\"windows\"";
        else if (generatorString.indexOf("Linux") >= 0)
            arguments += " opsys=\"unix|linux\"";
        else if (generatorString.indexOf("Unix") >= 0)
            arguments += " opsys=\"unix\"";
        else if (generatorString.indexOf("Solaris") >= 0) {
            if (generatorString.indexOf("x86") >= 0)
                arguments += " opsys=\"unix|solaris|x86\"";
            else
                arguments += " opsys=\"unix|solaris\"";
        }

        if (generatorString.indexOf("PowerPoint") >= 0)
            arguments += " program=\"microsoftoffice|powerpoint\"";
        else if (generatorString.indexOf("Excel") >= 0)
            arguments += " program=\"microsoftoffice|excel\"";
        else if (generatorString.indexOf("Word") >= 0)
            arguments += " program=\"microsoftoffice|word\"";
        else if (generatorString.indexOf("KOffice") >= 0)
            arguments += " program=\"koffice\"";
        else if (generatorString.indexOf("AbiWord") >= 0)
            arguments += " program=\"abiword\"";
        else if (generatorString.indexOf("LibreOffice") >= 0)
            arguments += " program=\"libreoffice\"";
        else if (generatorString.indexOf("Lotus Symphony") >= 0)
            arguments += " program=\"lotussymphony\"";
        else if (generatorString.indexOf("OpenOffice") >= 0) {
            if (generatorString.indexOf("StarOffice") >= 0)
                arguments += " program=\"openoffice|staroffice\"";
            else if (generatorString.indexOf("BrOffice") >= 0)
                arguments += " program=\"openoffice|broffice\"";
            else if (generatorString.indexOf("NeoOffice") >= 0)
                arguments += " program=\"openoffice|neooffice\"";
            else
                arguments += " program=\"openoffice\"";
        }

        QRegExp versionRegExp("/(\\d+(\\.\\d+(\\.\\d+)?)?)");
        if (versionRegExp.indexIn(generatorString) >= 0)
            arguments += QString(" version=\"%1\"").arg(versionRegExp.cap(1));

        return QString("<generator%2>%1</generator>\n").arg(DocScan::xmlify(generatorString)).arg(arguments);
    }

public:
    ODFMetaFileHandler(QString &logText)
        : QXmlDefaultHandler(), m_logText(logText) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);

        if (qName == "meta:document-statistic" && !atts.value("meta:page-count").isEmpty())
            m_logText += "<statistics type=\"pagecount\" origin=\"document\">" + atts.value("meta:page-count") + "</statistics>\n";
        if (qName == "office:document-meta" && !atts.value("office:version").isEmpty()) {
            QStringList versionNumbers = atts.value("office:version").split(".");
            m_logText += QString("<fileformat-version major=\"%1\" minor=\"%2\">%1.%2</fileformat-version>\n").arg(versionNumbers[0]).arg(versionNumbers[1]);
        }

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        QString nodeName = m_nodeName.top().toLower();

        QDate date = QDate::fromString(text.left(10), "yyyy-MM-dd");
        if (nodeName == "dc:title" || nodeName == "dc:subject")
            m_logText += QString("<%1>%2</%1>\n").arg(nodeName.replace(QRegExp("^(dc|meta):"), "")).arg(DocScan::xmlify(text));
        else if (nodeName == "meta:generator")
            m_logText += interpeteGenerator(text);
        else if (nodeName == "dc:creator" || nodeName == "meta:initial-creator")
            m_logText += QString("<meta name=\"%1\">%2</meta>\n").arg(nodeName.replace("creator", "author").replace(QRegExp("^(dc|meta):"), "")).arg(DocScan::xmlify(text));
        else if (nodeName == "dc:language")
            m_logText += QString("<meta name=\"%1\" origin=\"document\">%2</meta>\n").arg(nodeName.replace(QRegExp("^(dc|meta):"), "")).arg(DocScan::xmlify(text));
        else if (nodeName == "meta:keyword")
            m_logText += QString("<meta name=\"%1s\">%2</meta>\n").arg(nodeName.replace(QRegExp("^(dc|meta):"), "")).arg(DocScan::xmlify(text));
        else if (nodeName == "meta:creation-date" && date.isValid())
            m_logText += DocScan::formatDate(date, "creation");
        else if (nodeName == "dc:date" && date.isValid())
            m_logText += DocScan::formatDate(date, "modification");
        else if (nodeName == "meta:print-date" && date.isValid())
            m_logText += DocScan::formatDate(date, "print");

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
                mimetypeFile.close();
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

        if (zipFile.setCurrentFile("styles.xml", QuaZip::csInsensitive)) {
            QuaZipFile stylesXML(&zipFile, parent());
            analyzeStylesXML(stylesXML, logText);
        } else {
            emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-styles\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        if (!logText.contains("meta name=\"language\"") && zipFile.setCurrentFile("content.xml", QuaZip::csInsensitive)) {
            QuaZipFile contentXML(&zipFile, parent());
            QString plain = text(contentXML, logText);
            QString language;
            if (plain.length() > 1024 && !(language = guessLanguage(plain)).isEmpty())
                logText += "<meta name=\"language\" origin=\"aspell\">" + language + "</meta>\n";
        } else {
            emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-content\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        logText += "</fileanalysis>\n";

        emit analysisReport(logText);
        zipFile.close();
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

void FileAnalyzerODF::analyzeStylesXML(QIODevice &device, QString &logText)
{
    ODFStylesFileHandler handler(logText);
    QXmlInputSource source(&device);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(&source);
}

QString FileAnalyzerODF::text(QIODevice &device, QString &logText)
{
    QString result;
    ODFContentFileHandler handler(result, logText);
    QXmlInputSource source(&device);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(&source);
    return result;
}
