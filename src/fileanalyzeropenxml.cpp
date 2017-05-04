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

#include "fileanalyzeropenxml.h"

#include <quazip.h>
#include <quazipfile.h>

#include <QRegExp>
#include <QTextStream>
#include <QXmlDefaultHandler>
#include <QStack>
#include <QDebug>
#include <QFileInfo>

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
        result.formatVersion.clear(); // TODO
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);
        m_insideText |= qName == QStringLiteral("w:t");

        if (result.paperSizeWidth == 0 && qName == QStringLiteral("w:pgSz")) {
            bool ok = false;
            int mmw = atts.value(QStringLiteral("w:w")).toInt(&ok) / 56.695238f;
            if (ok) {
                result.paperSizeWidth = mmw;
                int mmh = atts.value(QStringLiteral("w:h")).toInt(&ok) / 56.695238f;
                if (ok)
                    result.paperSizeHeight = mmh;
            }
        } else if (qName == QStringLiteral("w:lang"))
            result.languageDocument = atts.value(QStringLiteral("w:eastAsia"));

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        if (qName == QStringLiteral("w:t")) m_insideText = false;
        if (qName == QStringLiteral("w:p")) result.plainText += QStringLiteral("\n");

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
        if (qName == QStringLiteral("w:themeFontLang"))
            m_language = atts.value(QStringLiteral("w:val"));

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        if (qName == QStringLiteral("w:document") && !m_language.isEmpty()) {
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
        if (qName == QStringLiteral("a:rPr"))
            m_language = atts.value(QStringLiteral("lang"));

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        if (qName == QStringLiteral("p:sld") && !m_language.isEmpty()) {
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
        if (m_nodeName.top() == QStringLiteral("dc:creator"))
            result.authorInitial = QString(QStringLiteral("<author type=\"first\">%1</author>\n")).arg(DocScan::xmlify(text));
        else if (m_nodeName.top() == QStringLiteral("cp:lastModifiedBy"))
            result.authorLast = QString(QStringLiteral("<author type=\"last\">%1</author>\n")).arg(DocScan::xmlify(text));
        else if (m_nodeName.top() == QStringLiteral("dcterms:created")) {
            QDate date = QDate::fromString(text.left(10), QStringLiteral("yyyy-MM-dd"));
            result.dateCreation = DocScan::formatDate(date, QStringLiteral("creation"));
        } else if (m_nodeName.top() == QStringLiteral("dcterms:modified")) {
            QDate date = QDate::fromString(text.left(10), QStringLiteral("yyyy-MM-dd"));
            result.dateModification = DocScan::formatDate(date, QStringLiteral("modification"));
        } else if (m_nodeName.top() == QStringLiteral("dc:title"))
            result.title = QString(QStringLiteral("<title>%1</title>\n")).arg(DocScan::xmlify(text));
        else if (m_nodeName.top() == QStringLiteral("dc:subject"))
            result.subject = QString(QStringLiteral("<subject>%1</subject>\n")).arg(DocScan::xmlify(text));

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

        if (qName == QStringLiteral("Properties")) {
            QString toolString = application;
            if (!toolString.contains(QRegExp(QStringLiteral("\\d\\.\\d"))))
                toolString.append(" " + appVersion); /// avoid duplicate version numbers
            QString guess = p->guessTool(toolString);
            if (!guess.isEmpty())
                result.toolGenerator = QString(QStringLiteral("<tool type=\"generator\">\n%1</tool>\n")).arg(guess);
        }
        return QXmlDefaultHandler::endElement(namespaceURI, localName, qName);
    }

    virtual bool characters(const QString &text) {
        if (m_nodeName.top() == QStringLiteral("Application"))
            application = text;
        else if (m_nodeName.top() == QStringLiteral("AppVersion"))
            appVersion = QString(text).replace(QStringLiteral("0000"), QStringLiteral("0"));
        else if (m_nodeName.top() == QStringLiteral("Characters")) {
            bool ok = false;
            result.characterCount = text.toInt(&ok);
            if (!ok) result.characterCount = 0;
        } else if (m_nodeName.top() == QStringLiteral("Pages")) {
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
        QString mimetype = QStringLiteral("application/octet-stream");
        if (zipFile.setCurrentFile(QStringLiteral("[Content_Types].xml"), QuaZip::csInsensitive)) {
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

        if (mimetype == QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document")) {
            if (!processWordFile(zipFile, result)) {
                emit analysisReport(QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"invalid-document\" status=\"error\" />\n")).arg(DocScan::xmlify(filename)));
                return;
            }
        }

        if (!processCore(zipFile, result)) {
            emit analysisReport(QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"invalid-corefile\" status=\"error\" />\n")).arg(DocScan::xmlify(filename)));
            return;
        }

        if (!processApp(zipFile, result)) {
            emit analysisReport(QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"invalid-appfile\" status=\"error\" />\n")).arg(DocScan::xmlify(filename)));
            return;
        }

        if (!processSettings(zipFile, result)) {
            if (!processSlides(zipFile, result)) {
                emit analysisReport(QString(QStringLiteral("<fileanalysis filename=\"%1\" status=\"error\" />\n")).arg(DocScan::xmlify(filename)));
                return;
            }
        }

        QString logText = QString(QStringLiteral("<fileanalysis filename=\"%1\" status=\"ok\">\n")).arg(DocScan::xmlify(filename));
        QString metaText = QStringLiteral("<meta>\n");
        QString headerText = QStringLiteral("<header>\n");

        /// file format including mime type and file format version
        metaText.append(QString(QStringLiteral("<fileformat>\n<mimetype>%1</mimetype>\n")).arg(mimetype));
        if (!result.formatVersion.isEmpty())
            metaText.append(QString(QStringLiteral("<version>%1</version>\n")).arg(result.formatVersion));
        metaText.append(QStringLiteral("</fileformat>"));

        /// file information including size
        QFileInfo fi = QFileInfo(filename);
        metaText.append(QString(QStringLiteral("<file size=\"%1\" />")).arg(fi.size()));

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
            headerText.append(QString(QStringLiteral("<language origin=\"document\">%1</language>\n")).arg(result.languageDocument));
        if (!result.languageAspell.isEmpty())
            headerText.append(QString(QStringLiteral("<language origin=\"aspell\">%1</language>\n")).arg(result.languageAspell));

        /// evaluate paper size
        if (result.paperSizeHeight > 0 && result.paperSizeWidth > 0)
            headerText.append(evaluatePaperSize(result.paperSizeWidth, result.paperSizeHeight));

        /// evaluate number of pages
        if (result.pageCount > 0)
            headerText.append(QString(QStringLiteral("<num-pages origin=\"document\">%1</num-pages>\n")).arg(result.pageCount));

        /// evaluate dates
        if (!result.dateCreation.isEmpty())
            headerText.append(result.dateCreation);
        if (!result.dateModification.isEmpty())
            headerText.append(result.dateModification);

        // TODO fonts

        QString bodyText = QString(QStringLiteral("<body length=\"%1\" />\n")).arg(result.characterCount);

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
        emit analysisReport(QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" />\n")).arg(DocScan::xmlify(filename)));

    m_isAlive = false;
}

bool FileAnalyzerOpenXML::processWordFile(QuaZip &zipFile, ResultContainer &result)
{
    if (zipFile.setCurrentFile(QStringLiteral("word/document.xml"), QuaZip::csInsensitive)) {
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
    if (zipFile.setCurrentFile(QStringLiteral("docProps/core.xml"), QuaZip::csInsensitive)) {
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
    if (zipFile.setCurrentFile(QStringLiteral("docProps/app.xml"), QuaZip::csInsensitive)) {
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
    if (zipFile.setCurrentFile(QStringLiteral("word/settings.xml"), QuaZip::csInsensitive)) {
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
    if (zipFile.setCurrentFile(QStringLiteral("ppt/slides/slide1.xml"), QuaZip::csInsensitive)) {
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
