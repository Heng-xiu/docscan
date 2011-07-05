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


class FileAnalyzerOpenXML::OpenXMLDocumentHandler: public QXmlDefaultHandler
{
private:
    FileAnalyzerOpenXML *p;
    ResultContainer &result;
    QStack<QString> m_nodeName;
    bool m_insideText;
    QStringList m_fontNames;
    QString c;

public:
    OpenXMLDocumentHandler(FileAnalyzerOpenXML *parent, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), p(parent), result(resultContainer), m_insideText(false) {
        // nothing
    }

    virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) {
        m_nodeName.push(qName);
        m_insideText |= qName == "w:t";

        if (qName == "w:rFonts") {
            QString fontName = atts.value("w:ascii");
            if (!fontName.isEmpty() && !m_fontNames.contains(fontName))
                m_fontNames.append(fontName);
        } else if (qName == "w:pgSz") {
            bool ok = false;
            int mmw = atts.value("w:w").toInt(&ok) / 56.695238f;
            if (ok) {
                result.paperSizeWidth = mmw;
                int mmh = atts.value("w:h").toInt(&ok) / 56.695238f;
                if (ok)
                    result.paperSizeHeight = mmh;
            }
        } else if (qName == "w:lang")
            result.languageDocument = atts.value("w:eastAsia");

        return QXmlDefaultHandler::startElement(namespaceURI, localName, qName, atts);
    }

    virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) {
        m_nodeName.pop();
        if (qName == "w:t") m_insideText = false;
        if (qName == "w:p") result.plainText += "\n";
        if (qName == "w:document") {
            if (!m_fontNames.isEmpty()) {
                /* FIXME
                m_logText += QString("<statistics type=\"fonts\" origin=\"document\" count=\"%1\">\n").arg(m_fontNames.count());
                foreach(QString fontName, m_fontNames) {
                    m_logText += QString("<font name=\"%1\" />\n").arg(DocScan::xmlify(fontName));
                }
                m_logText += "</statistics>\n";
                */
            }
        }
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
    FileAnalyzerOpenXML *p;
    ResultContainer &result;
    QString m_language;

public:
    OpenXMLSettingsHandler(FileAnalyzerOpenXML *parent, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), p(parent), result(resultContainer), m_language(QString::null) {
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
    FileAnalyzerOpenXML *p;
    ResultContainer &result;
    QString m_language;

public:
    OpenXMLSlideHandler(FileAnalyzerOpenXML *parent, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), p(parent), result(resultContainer), m_language(QString::null) {
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
    FileAnalyzerOpenXML *p;
    ResultContainer &result;
    QStack<QString> m_nodeName;

public:
    OpenXMLCoreHandler(FileAnalyzerOpenXML *parent, ResultContainer &resultContainer)
        : QXmlDefaultHandler(), p(parent), result(resultContainer)  {
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
        }

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

        return QString("<generator%2 license=\"%3\">%1</generator>\n").arg(DocScan::xmlify(generatorString)).arg(arguments).arg("" /*p->guessLicenseFromProduct(generatorString)*/);
    }

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
            QString guess = p->guessTool(application + " " + appVersion);
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
                emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-document\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
                return;
            }
        }

        if (mimetype == "application/vnd.openxmlformats-officedocument.wordprocessingml.document") {
            if (!processWordFile(zipFile, result))
                emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-document\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
        }

        if (!processCore(zipFile, result)) {
            emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-corefile\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        if (!processApp(zipFile, result)) {
            emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-appfile\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
            return;
        }

        if (!processSettings(zipFile, result)) {
            if (!processSlides(zipFile, result)) {
                emit analysisReport(QString("<fileanalysis status=\"error\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));
                return;
            }
        }

        QString logText = QString("<fileanalysis status=\"ok\" filename=\"%1\">\n").arg(DocScan::xmlify(filename));
        QString metaText = QLatin1String("<meta>\n");
        QString headerText = QLatin1String("<header>\n");


        /// file format including mime type and file format version
        // TODO document version
        metaText.append(QString("<fileformat>\n<mimetype>%1</mimetype>\n</fileformat>").arg(mimetype));


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
        // TODO if (!result.title.isEmpty())
        // TODO     headerText.append(result.title);

        /// evaluate subject
        // TODO if (!result.subject.isEmpty())
        // TODO     headerText.append(result.subject);

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
        metaText += QLatin1String("</meta>\n");
        logText.append(metaText);
        headerText += QLatin1String("</header>\n");
        logText.append(headerText);
        logText.append(bodyText);
        logText += QLatin1String("</fileanalysis>\n");

        emit analysisReport(logText);

        zipFile.close();
    } else
        emit analysisReport(QString("<fileanalysis status=\"error\" message=\"invalid-fileformat\" filename=\"%1\" />\n").arg(DocScan::xmlify(filename)));

    m_isAlive = false;
}

bool FileAnalyzerOpenXML::processWordFile(QuaZip &zipFile, ResultContainer &result)
{
    if (zipFile.setCurrentFile("word/document.xml", QuaZip::csInsensitive)) {
        QuaZipFile documentFile(&zipFile, parent());
        if (documentFile.open(QIODevice::ReadOnly)) {
            text(documentFile, result);
            if (result.plainText.length() > 1024)
                result.languageAspell = guessLanguage(result.plainText);
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
