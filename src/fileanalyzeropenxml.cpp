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

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>

#include <QRegExp>
#include <QTextStream>
#include <QXmlDefaultHandler>
#include <QStack>
#include <QDebug>
#include <QFileInfo>

#include "fileanalyzeropenxml.h"
#include "general.h"


class FileAnalyzerOpenXML::OpenXMLDocumentHandler: public QXmlDefaultHandler
{
private:
    ResultContainer &result;
    QStack<QString> m_nodeName;
    bool m_insideText;
    QString c;

public:
    OpenXMLDocumentHandler(FileAnalyzerOpenXML *, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), result(resultContainer), m_insideText(false) {
        result.paperSizeWidth = result.paperSizeHeight = 0;
        result.formatVersion = QStringLiteral(""); // TODO
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);
        m_insideText |= qName == "w:t";

        if (result.paperSizeWidth == 0 && qName == QStringLiteral("w:pgSz")) {
            bool ok = false;
            int mmw = atts.value(QStringLiteral("w:w")).toInt(&ok) / 56.695238f;
            if (ok) {
                result.paperSizeWidth = mmw;
                int mmh = atts.value("w:h").toInt(&ok) / 56.695238f;
                if (ok)
                    result.paperSizeHeight = mmh;
            }
        } else if (qName == QStringLiteral("w:lang"))
            result.languageDocument = atts.value(QStringLiteral("w:eastAsia"));

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        if (qName == "w:t") m_insideText = false;
        if (qName == "w:p") result.plainText += "\n";

        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        if (m_insideText) result.plainText += text;
        return QXmlDefaultHandler::characters(text);
    }
};

class FileAnalyzerOpenXML::OpenXMLSettingsHandler: public QXmlDefaultHandler
{
private:
    ResultContainer &result;
    QString m_language;

public:
    OpenXMLSettingsHandler(FileAnalyzerOpenXML *, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), result(resultContainer), m_language(QString::null) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        if (qName == "w:themeFontLang")
            m_language = atts.value("w:val");

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        if (qName == "w:document" && !m_language.isEmpty()) {
            result.languageDocument = m_language;
        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }
};

class FileAnalyzerOpenXML::OpenXMLSlideHandler: public QXmlDefaultHandler
{
private:
    ResultContainer &result;
    QString m_language;

public:
    OpenXMLSlideHandler(FileAnalyzerOpenXML *, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), result(resultContainer), m_language(QString::null) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        if (qName == "a:rPr")
            m_language = atts.value("lang");

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        if (qName == "p:sld" && !m_language.isEmpty()) {
            result.languageDocument = m_language;

        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }
};

class FileAnalyzerOpenXML::OpenXMLCoreHandler: public QXmlDefaultHandler
{
private:
    ResultContainer &result;
    QStack<QString> m_nodeName;

public:
    OpenXMLCoreHandler(FileAnalyzerOpenXML *, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), result(resultContainer)  {
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
            result.authorInitial = QString("<author type=\"first\">%1</author>\n").arg(DocScan::xmlify(text));
        else if (m_nodeName.top() == "cp:lastModifiedBy")
            result.authorLast = QString("<author type=\"last\">%1</author>\n").arg(DocScan::xmlify(text));
        else if (m_nodeName.top() == "dcterms:created") {
            QDate date = QDate::fromString(text.left(10), "yyyy-MM-dd");
            result.dateCreation = DocScan::formatDate(date, "creation");
        } else if (m_nodeName.top() == "dcterms:modified") {
            QDate date = QDate::fromString(text.left(10), "yyyy-MM-dd");
            result.dateModification = DocScan::formatDate(date, "modification");
        } else if (m_nodeName.top() == "dc:title")
            result.title = QString("<title>%1</title>\n").arg(DocScan::xmlify(text));
        else if (m_nodeName.top() == "dc:subject")
            result.subject = QString("<subject>%1</subject>\n").arg(DocScan::xmlify(text));

        return QXmlDefaultHandler::characters(text);
    }
};

class FileAnalyzerOpenXML::OpenXMLAppHandler: public QXmlDefaultHandler
{
private:
    FileAnalyzerOpenXML *p;
    ResultContainer &result;
    QStack<QString> m_nodeName;
    QString application, appVersion;

public:
    OpenXMLAppHandler(FileAnalyzerOpenXML *parent, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), p(parent), result(resultContainer) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);
        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();

        if (qName == "Properties") {
            QString toolString = application;
            if (!toolString.contains(QRegExp("\\d\\.\\d")))
                toolString.append(" " + appVersion); /// avoid duplicate version numbers
            QString guess = p->guessTool(toolString);
            if (!guess.isEmpty())
                result.toolGenerator = QString("<tool type=\"generator\">\n%1</tool>\n").arg(guess);
        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        if (m_nodeName.top() == "Application")
            application = text;
        else if (m_nodeName.top() == "AppVersion")
            appVersion = QString(text).replace("0000", "0");
        else if (m_nodeName.top() == "Characters") {
            bool ok = false;
            result.characterCount = text.toInt(&ok);
            if (!ok) result.characterCount = 0;
        } else if (m_nodeName.top() == "Pages") {
            bool ok = false;
            result.pageCount = text.toInt(&ok);
            if (!ok) result.pageCount = 0;
        }


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
    ResultContainer result;
    result.characterCount = 0;
    result.pageCount = 0;
    result.paperSizeHeight = result.paperSizeWidth = 0;

    m_isAlive = true;
    QuaZip zipFile(filename);

    if (zipFile.open(QuaZip::mdUnzip)) {

        /// determine mime type
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

        if (mimetype == "application/vnd.openxmlformats-officedocument.wordprocessingml.document") {
            if (!processWordFile(zipFile, result)) {
                emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-document\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
                return;
            }
        }

        if (!processCore(zipFile, result)) {
            emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-corefile\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        if (!processApp(zipFile, result)) {
            emit analysisReport(QString("<fileanalysis filename=\"%1\" message=\"invalid-appfile\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        if (!processSettings(zipFile, result)) {
            if (!processSlides(zipFile, result)) {
                emit analysisReport(QString("<fileanalysis filename=\"%1\" status=\"error\" />\n").arg(DocScan::xmlify(filename)));
                return;
            }
        }

        QString logText = QString("<fileanalysis filename=\"%1\" status=\"ok\">\n").arg(DocScan::xmlify(filename));
        QString metaText = QStringLiteral("<meta>\n");
        QString headerText = QStringLiteral("<header>\n");

        /// file format including mime type and file format version
        metaText.append(QString("<fileformat>\n<mimetype>%1</mimetype>\n").arg(mimetype));
        if (!result.formatVersion.isEmpty())
            metaText.append(QString("<version>%1</version>\n").arg(result.formatVersion));
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
        if (!result.languageDocument.isEmpty())
            headerText.append(QString("<language origin=\"document\">%1</language>\n").arg(result.languageDocument));
        if (!result.languageAspell.isEmpty())
            headerText.append(QString("<language origin=\"aspell\">%1</language>\n").arg(result.languageAspell));

        /// evaluate paper size
        if (result.paperSizeHeight > 0 && result.paperSizeWidth > 0)
            headerText.append(evaluatePaperSize(result.paperSizeWidth, result.paperSizeHeight));

        /// evaluate number of pages
        if (result.pageCount > 0)
            headerText.append(QString("<num-pages origin=\"document\">%1</num-pages>\n").arg(result.pageCount));

        /// evaluate dates
        if (!result.dateCreation.isEmpty())
            headerText.append(result.dateCreation);
        if (!result.dateModification.isEmpty())
            headerText.append(result.dateModification);

        // TODO fonts

        QString bodyText = QString("<body length=\"%1\" />\n").arg(result.characterCount);

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

bool FileAnalyzerOpenXML::processWordFile(QuaZip &zipFile, ResultContainer &result)
{
    if (zipFile.setCurrentFile("word/document.xml", QuaZip::csInsensitive)) {
        QuaZipFile documentFile(&zipFile, parent());
        if (documentFile.open(QIODevice::ReadOnly)) {
            text(documentFile, result);
            /* Disabling aspell, computationally expensive
            if (result.plainText.length() > 1024)
                result.languageAspell = guessLanguage(result.plainText);
             */
            documentFile.close();
            return true;
        }
    }
    return false;
}


bool FileAnalyzerOpenXML::processCore(QuaZip &zipFile, ResultContainer &result)
{
    if (zipFile.setCurrentFile("docProps/core.xml", QuaZip::csInsensitive)) {
        QuaZipFile coreFile(&zipFile, parent());
        if (coreFile.open(QIODevice::ReadOnly)) {
            OpenXMLCoreHandler handler(this, result);
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

bool FileAnalyzerOpenXML::processApp(QuaZip &zipFile, ResultContainer &result)
{
    if (zipFile.setCurrentFile("docProps/app.xml", QuaZip::csInsensitive)) {
        QuaZipFile appFile(&zipFile, parent());
        if (appFile.open(QIODevice::ReadOnly)) {
            OpenXMLAppHandler handler(this, result);
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

bool FileAnalyzerOpenXML::processSettings(QuaZip &zipFile, ResultContainer &result)
{
    if (zipFile.setCurrentFile("word/settings.xml", QuaZip::csInsensitive)) {
        QuaZipFile appFile(&zipFile, parent());
        if (appFile.open(QIODevice::ReadOnly)) {
            OpenXMLSettingsHandler handler(this, result);
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

bool FileAnalyzerOpenXML::processSlides(QuaZip &zipFile, ResultContainer &result)
{
    if (zipFile.setCurrentFile("ppt/slides/slide1.xml", QuaZip::csInsensitive)) {
        QuaZipFile appFile(&zipFile, parent());
        if (appFile.open(QIODevice::ReadOnly)) {
            OpenXMLSlideHandler handler(this, result);
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

void FileAnalyzerOpenXML::text(QIODevice &device, ResultContainer &result)
{
    OpenXMLDocumentHandler handler(this, result);
    QXmlInputSource source(&device);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(&source);
}
