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

#include <QRegExp>
#include <QTextStream>
#include <QXmlDefaultHandler>
#include <QStack>
#include <QDebug>
#include <QFileInfo>

#include "fileanalyzeropenxml.h"
#include "general.h"


class OpenXMLDocumentHandler: public QXmlDefaultHandler
{
private:
    QString &m_text;
    QString &m_logText;
    QStack<QString> m_nodeName;
    bool m_insideText;
    QStringList m_fontNames;
    QString c;

public:
    OpenXMLDocumentHandler(QString &text, QString &logText)
        : QXmlDefaultHandler(), m_text(text), m_logText(logText), m_insideText(false) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);
        m_insideText |= qName == "w:t";

        if (qName == "w:rFonts") {
            QString fontName = atts.value("w:ascii");
            if (!fontName.isEmpty() && !m_fontNames.contains(fontName))
                m_fontNames.append(fontName);
        }

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        if (qName == "w:t") m_insideText = false;
        if (qName == "w:p" && m_text.length() < 16384) m_text += "\n";
        if (qName == "w:document") {
            if (!m_fontNames.isEmpty()) {
                m_logText += QString("<statistics type=\"fonts\" origin=\"document\" count=\"%1\">\n").arg(m_fontNames.count());
                foreach(QString fontName, m_fontNames) {
                    m_logText += QString("<font name=\"%1\" />\n").arg(DocScan::xmlify(fontName));
                }
                m_logText += "</statistics>\n";
            }
        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        if (m_insideText && m_text.length() < 16384) m_text += text;
        return QXmlDefaultHandler::characters(text);
    }
};

class OpenXMLSettingsHandler: public QXmlDefaultHandler
{
private:
    QString &m_logText;
    QString m_language;

public:
    OpenXMLSettingsHandler(QString &logText)
        : QXmlDefaultHandler(), m_logText(logText), m_language(QString::null) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        if (qName == "w:themeFontLang")
            m_language = atts.value("w:val");

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        if (qName == "w:document" && !m_language.isEmpty()) {
            m_logText += "<meta name=\"language\" origin=\"document\">" + m_language + "</meta>\n";

        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }
};

class OpenXMLSlideHandler: public QXmlDefaultHandler
{
private:
    QString &m_logText;
    QString m_language;

public:
    OpenXMLSlideHandler(QString &logText)
        : QXmlDefaultHandler(), m_logText(logText), m_language(QString::null) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        if (qName == "a:rPr")
            m_language = atts.value("lang");

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        if (qName == "p:sld" && !m_language.isEmpty()) {
            m_logText += "<meta name=\"language\" origin=\"document\">" + m_language + "</meta>\n";

        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }
};

class OpenXMLCoreHandler: public QXmlDefaultHandler
{
private:
    QString &m_logText;
    QStack<QString> m_nodeName;

public:
    OpenXMLCoreHandler(QString &logText)
        : QXmlDefaultHandler(), m_logText(logText) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);
        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        if (m_nodeName.top() == "dc:creator")
            m_logText += QString("<meta name=\"initial-creator\">%1</meta>\n").arg(DocScan::xmlify(text));
        else if (m_nodeName.top() == "cp:lastModifiedBy")
            m_logText += QString("<meta name=\"creator\">%1</meta>\n").arg(DocScan::xmlify(text));
        else if (m_nodeName.top() == "dcterms:created") {
            QDate date = QDate::fromString(text.left(10), "yyyy-MM-dd");
            m_logText += DocScan::formatDate(date, "creation");
        } else if (m_nodeName.top() == "dcterms:modified") {
            QDate date = QDate::fromString(text.left(10), "yyyy-MM-dd");
            m_logText += DocScan::formatDate(date, "modification");
        }

        return QXmlDefaultHandler::characters(text);
    }
};

class OpenXMLAppHandler: public QXmlDefaultHandler
{
private:
    QString &m_logText;
    QStack<QString> m_nodeName;

    QString interpeteGenerator(const QString &generatorString) {
        QString arguments;

        if (generatorString.indexOf("Macintosh") >= 0)
            arguments += " opsys=\"unix|macos\"";
        else
            arguments += " opsys=\"windows\"";

        if (generatorString.indexOf("PowerPoint") >= 0)
            arguments += " program=\"microsoftoffice|powerpoint\"";
        else if (generatorString.indexOf("Excel") >= 0)
            arguments += " program=\"microsoftoffice|excel\"";
        else if (generatorString.indexOf("Word") >= 0)
            arguments += " program=\"microsoftoffice|word\"";

        QRegExp versionRegExp("(\\d+(\\.\\d+(\\.\\d+)?)?)");
        if (versionRegExp.indexIn(generatorString) >= 0)
            arguments += QString(" version=\"%1\"").arg(versionRegExp.cap(1));

        return QString("<generator%2 license=\"%3\">%1</generator>\n").arg(DocScan::xmlify(generatorString)).arg(arguments).arg(FileAnalyzerAbstract::guessLicenseFromProduct(generatorString));
    }

public:
    OpenXMLAppHandler(QString &logText)
        : QXmlDefaultHandler(), m_logText(logText) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);
        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        if (m_nodeName.top() == "Application")
            m_logText += interpeteGenerator(text);
        return QXmlDefaultHandler::characters(text);
    }
};

FileAnalyzerOpenXML::FileAnalyzerOpenXML(QObject *parent)
    : FileAnalyzerAbstract(parent), m_isAlive(false)
{
}

bool FileAnalyzerOpenXML::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerOpenXML::analyzeFile(const QString &filename)
{
    m_isAlive = true;
    QuaZip zipFile(filename);

    if (zipFile.open(QuaZip::mdUnzip)) {
        QString mimetype = "application/octet-stream";

        if (zipFile.setCurrentFile("[Content_Types].xml", QuaZip::csInsensitive)) {
            QuaZipFile contentTypeFile(&zipFile, parent());
            if (contentTypeFile.open(QIODevice::ReadOnly)) {
                QTextStream ts(&contentTypeFile);
                QString allText = ts.readAll();
                QRegExp mainMimeTypeRegExp("ContentType=\"(application/vnd.openxmlformats-officedocument.[^\"]+).main[+]xml\"");
                if (mainMimeTypeRegExp.indexIn(allText) >= 0)
                    mimetype = mainMimeTypeRegExp.cap(1);
                contentTypeFile.close();
            }
        }

        QString logText = QString("<fileanalysis mimetype=\"%1\" filename=\"%2\">\n").arg(mimetype).arg(DocScan::xmlify(filename));

        if (mimetype == "application/vnd.openxmlformats-officedocument.wordprocessingml.document") {
            if (!processWordFile(zipFile, logText))
                emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-document\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
        }

        if (!processCore(zipFile, logText))
            emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-corefile\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
        if (!processApp(zipFile, logText))
            emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-appfile\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
        processSettings(zipFile, logText) || processSlides(zipFile, logText);

        logText += "<statistics type=\"size\" unit=\"bytes\">" + QString::number(QFileInfo(filename).size()) + "</statistics>\n";
        logText += "</fileanalysis>\n";

        emit analysisReport(logText);

        zipFile.close();
    } else
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-fileformat\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));

    m_isAlive = false;
}

bool FileAnalyzerOpenXML::processWordFile(QuaZip &zipFile, QString &logText)
{
    if (zipFile.setCurrentFile("word/document.xml", QuaZip::csInsensitive)) {
        QuaZipFile documentFile(&zipFile, parent());
        if (documentFile.open(QIODevice::ReadOnly)) {
            QString plain = text(documentFile, logText);
            QString language;
            if (plain.length() > 1024 && !(language = guessLanguage(plain)).isEmpty())
                logText += "<meta name=\"language\" origin=\"aspell\">" + language + "</meta>\n";

            documentFile.close();
            return true;
        }
    }
    return false;
}


bool FileAnalyzerOpenXML::processCore(QuaZip &zipFile, QString &logText)
{
    if (zipFile.setCurrentFile("docProps/core.xml", QuaZip::csInsensitive)) {
        QuaZipFile coreFile(&zipFile, parent());
        if (coreFile.open(QIODevice::ReadOnly)) {
            QString result;
            OpenXMLCoreHandler handler(logText);
            QXmlInputSource source(&coreFile);
            QXmlSimpleReader reader;
            reader.setContentHandler(&handler);
            reader.parse(&source);

            coreFile.close();
            return true;
        }
    }
    return false;
}

bool FileAnalyzerOpenXML::processApp(QuaZip &zipFile, QString &logText)
{
    if (zipFile.setCurrentFile("docProps/app.xml", QuaZip::csInsensitive)) {
        QuaZipFile appFile(&zipFile, parent());
        if (appFile.open(QIODevice::ReadOnly)) {
            QString result;
            OpenXMLAppHandler handler(logText);
            QXmlInputSource source(&appFile);
            QXmlSimpleReader reader;
            reader.setContentHandler(&handler);
            reader.parse(&source);

            appFile.close();
            return true;
        }
    }
    return false;
}

bool FileAnalyzerOpenXML::processSettings(QuaZip &zipFile, QString &logText)
{
    if (zipFile.setCurrentFile("word/settings.xml", QuaZip::csInsensitive)) {
        QuaZipFile appFile(&zipFile, parent());
        if (appFile.open(QIODevice::ReadOnly)) {
            QString result;
            OpenXMLSettingsHandler handler(logText);
            QXmlInputSource source(&appFile);
            QXmlSimpleReader reader;
            reader.setContentHandler(&handler);
            reader.parse(&source);

            appFile.close();
            return true;
        }
    }
    return false;
}

bool FileAnalyzerOpenXML::processSlides(QuaZip &zipFile, QString &logText)
{
    if (zipFile.setCurrentFile("ppt/slides/slide1.xml", QuaZip::csInsensitive)) {
        QuaZipFile appFile(&zipFile, parent());
        if (appFile.open(QIODevice::ReadOnly)) {
            QString result;
            OpenXMLSlideHandler handler(logText);
            QXmlInputSource source(&appFile);
            QXmlSimpleReader reader;
            reader.setContentHandler(&handler);
            reader.parse(&source);

            appFile.close();
            return true;
        }
    }
    return false;
}

QString FileAnalyzerOpenXML::text(QIODevice &device, QString &logText)
{
    QString result;
    OpenXMLDocumentHandler handler(result, logText);
    QXmlInputSource source(&device);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(&source);
    return result;
}
