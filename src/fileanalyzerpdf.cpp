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


    Copyright (2017) Thomas Fischer <thomas.fischer@his.se>, senior
    lecturer at University of Skövde, as part of the LIM-IT project.

 */

#include "fileanalyzerpdf.h"

#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QRegularExpression>
#include <QStandardPaths>

#include "watchdog.h"
#include "guessing.h"
#include "general.h"

static const int oneMinuteInMillisec = 60000;
static const int twoMinutesInMillisec = oneMinuteInMillisec * 2;
static const int fourMinutesInMillisec = oneMinuteInMillisec * 4;
static const int sixMinutesInMillisec = oneMinuteInMillisec * 6;

FileAnalyzerPDF::FileAnalyzerPDF(QObject *parent)
    : FileAnalyzerAbstract(parent), JHoveWrapper(), m_isAlive(false)
{
    FileAnalyzerAbstract::setObjectName(QStringLiteral("fileanalyzerpdf"));
}

bool FileAnalyzerPDF::isAlive()
{
    return m_isAlive;
}

void FileAnalyzerPDF::setupJhove(const QString &shellscript) {
    const QString report = JHoveWrapper::setupJhove(this, shellscript);
    if (!report.isEmpty()) {
        /// This was the first time 'setupJhove' inherited from JHoveWrapper was called
        /// and it gave us a report back. Propagate this report to the logging system
        emit analysisReport(QStringLiteral("jhovewrapper"), report);
    }
}

void FileAnalyzerPDF::setupVeraPDF(const QString &cliTool)
{
    m_veraPDFcliTool = cliTool;

    if (!m_veraPDFcliTool.isEmpty()) {
        QProcess veraPDF(this);
        const QStringList arguments = QStringList() << QStringLiteral("--version");
        veraPDF.start(m_veraPDFcliTool, arguments, QIODevice::ReadOnly);
        const bool veraPDFStarted = veraPDF.waitForStarted(oneMinuteInMillisec);
        if (!veraPDFStarted)
            qWarning() << "Failed to start veraPDF to retrieve version";
        else {
            const bool veraPDFExited = veraPDF.waitForFinished(oneMinuteInMillisec);
            if (!veraPDFExited)
                qWarning() << "Failed to finish veraPDF to retrieve version";
            else {
                const int veraPDFExitCode = veraPDF.exitCode();
                const QString veraPDFStandardOutput = QString::fromUtf8(veraPDF.readAllStandardOutput().constData()).trimmed();
                const QString veraPDFErrorOutput = QString::fromUtf8(veraPDF.readAllStandardError().constData()).trimmed();
                static const QRegularExpression regExpVersionNumber(QStringLiteral("veraPDF (([0-9]+[.])+[0-9]+)"));
                const QRegularExpressionMatch regExpVersionNumberMatch = regExpVersionNumber.match(veraPDFStandardOutput);
                const bool status = veraPDFExitCode == 0 && regExpVersionNumberMatch.hasMatch();
                const QString versionNumber = status ? regExpVersionNumberMatch.captured(1) : QString();

                QString report = QString(QStringLiteral("<toolcheck name=\"verapdf\" exitcode=\"%1\" status=\"%2\"%3>\n")).arg(veraPDFExitCode).arg(status ? QStringLiteral("ok") : QStringLiteral("error")).arg(status ? QString(QStringLiteral(" version=\"%1\"")).arg(versionNumber) : QString());
                if (!veraPDFStandardOutput.isEmpty())
                    report.append(QStringLiteral("<output>")).append(DocScan::xmlify(veraPDFStandardOutput)).append(QStringLiteral("</output>\n"));
                if (!veraPDFErrorOutput.isEmpty())
                    report.append(QStringLiteral("<error>")).append(DocScan::xmlify(veraPDFErrorOutput)).append(QStringLiteral("</error>\n"));
                report.append(QStringLiteral("</toolcheck>\n"));
                emit analysisReport(objectName(), report);
            }
        }
    }
}

void FileAnalyzerPDF::setupPdfBoXValidator(const QString &pdfboxValidatorJavaClass) {
    m_pdfboxValidatorJavaClass = pdfboxValidatorJavaClass;

    const QFileInfo fi(m_pdfboxValidatorJavaClass);
    if (fi.isFile()) {
        const QDir dir = fi.dir();
        const QStringList jarList = dir.entryList(QStringList() << QStringLiteral("pdfbox-*.jar"), QDir::Files);
        static const QRegularExpression regExpVersionNumber(QStringLiteral("pdfbox-(([0-9]+[.])+[0-9]+)\\.jar"));
        const QRegularExpressionMatch regExpVersionNumberMatch = jarList.count() > 0 ? regExpVersionNumber.match(jarList.first()) : QRegularExpressionMatch();
        const bool status = jarList.count() == 1 && regExpVersionNumberMatch.hasMatch();
        const QString versionNumber = status ? regExpVersionNumberMatch.captured(1) : QString();
        const QString report = QString(QStringLiteral("<toolcheck name=\"pdfboxvalidator\" status=\"%1\"%2 />\n")).arg(status ? QStringLiteral("ok") : QStringLiteral("error")).arg(status ? QString(QStringLiteral(" version=\"%1\"")).arg(versionNumber) : QString());
        emit analysisReport(objectName(), report);
    }
}

void FileAnalyzerPDF::setupCallasPdfAPilotCLI(const QString &callasPdfAPilotCLI) {
    m_callasPdfAPilotCLI = callasPdfAPilotCLI;

    // TODO version number logging
}

bool FileAnalyzerPDF::popplerAnalysis(const QString &filename, QString &logText, QString &metaText) {
    Poppler::Document *popplerDocument = Poppler::Document::load(filename);
    const bool popplerWrapperOk = popplerDocument != nullptr;
    if (popplerWrapperOk) {
        QString guess, headerText;
        const int numPages = popplerDocument->numPages();

        /// file format including mime type and file format version
        int majorVersion = 0, minorVersion = 0;
        popplerDocument->getPdfVersion(&majorVersion, &minorVersion);
        metaText.append(QString(QStringLiteral("<fileformat>\n<mimetype>application/pdf</mimetype>\n<version major=\"%1\" minor=\"%2\">%1.%2</version>\n<security locked=\"%3\" encrypted=\"%4\" />\n</fileformat>\n")).arg(QString::number(majorVersion), QString::number(minorVersion), popplerDocument->isLocked() ? QStringLiteral("yes") : QStringLiteral("no"), popplerDocument->isEncrypted() ? QStringLiteral("yes") : QStringLiteral("no")));

        if (enableEmbeddedFilesAnalysis) {
            metaText.append(QStringLiteral("<embeddedfiles>\n"));
            metaText.append(QStringLiteral("<parentfilename>") + DocScan::xmlify(filename) + QStringLiteral("</parentfilename>\n"));
            extractImages(metaText, filename);
            extractEmbeddedFiles(metaText, popplerDocument);
            metaText.append(QStringLiteral("</embeddedfiles>\n"));
        }

        /// guess and evaluate editor (a.k.a. creator)
        QString toolXMLtext;
        QString creator = popplerDocument->info(QStringLiteral("Creator"));
        guess.clear();
        if (!creator.isEmpty())
            guess = guessTool(creator, popplerDocument->info(QStringLiteral("Title")));
        if (!guess.isEmpty())
            toolXMLtext.append(QString(QStringLiteral("<tool type=\"editor\">\n%1</tool>\n")).arg(guess));
        /// guess and evaluate producer
        QString producer = popplerDocument->info(QStringLiteral("Producer"));
        guess.clear();
        if (!producer.isEmpty())
            guess = guessTool(producer, popplerDocument->info(QStringLiteral("Title")));
        if (!guess.isEmpty())
            toolXMLtext.append(QString(QStringLiteral("<tool type=\"producer\">\n%1</tool>\n")).arg(guess));
        if (!toolXMLtext.isEmpty())
            metaText.append(QStringLiteral("<tools>\n")).append(toolXMLtext).append(QStringLiteral("</tools>\n"));

        QHash<QString, struct ExtendedFontInfo> knownFonts;
        for (int pageNumber = 0; pageNumber < numPages;) {
            Poppler::FontIterator *fontIterator = popplerDocument->newFontIterator(pageNumber);
            ++pageNumber;
            if (fontIterator == nullptr) continue;

            if (fontIterator->hasNext()) {
                const QList<Poppler::FontInfo> fontList = fontIterator->next();
                for (const Poppler::FontInfo &fi : fontList) {
                    const QString fontName = fi.name();
                    if (knownFonts.contains(fontName)) {
                        struct ExtendedFontInfo &efi = knownFonts[fontName];
                        efi.recordOccurrence(pageNumber);
                    } else {
                        const struct ExtendedFontInfo efi(fi, pageNumber);
                        knownFonts[fontName] = efi;
                    }
                }
            }
            delete fontIterator; ///< clean memory
        }
        QString fontXMLtext;
        for (QHash<QString, struct ExtendedFontInfo>::ConstIterator it = knownFonts.constBegin(); it != knownFonts.constEnd(); ++it) {
            bool oninnerpage = false;
            for (int pageNumber = 4; !oninnerpage && pageNumber < numPages - 4; ++pageNumber)
                oninnerpage = it.value().pageNumbers.contains(pageNumber);
            fontXMLtext.append(QString(QStringLiteral("<font firstpage=\"%5\" lastpage=\"%6\" oninnerpage=\"%7\" embedded=\"%2\" subset=\"%3\"%4>\n%1</font>\n")).arg(Guessing::fontToXML(it.value().name, it.value().typeName), it.value().isEmbedded ? QStringLiteral("yes") : QStringLiteral("no"), it.value().isSubset ? QStringLiteral("yes") : QStringLiteral("no"), it.value().fileName.isEmpty() ? QString() : QString(QStringLiteral(" filename=\"%1\"")).arg(it.value().fileName)).arg(it.value().firstPageNumber).arg(it.value().lastPageNumber).arg(oninnerpage ? QStringLiteral("yes") : QStringLiteral("no")));
        }
        if (!fontXMLtext.isEmpty())
            /// Wrap multiple <font> tags into one <fonts> tag
            metaText.append(QStringLiteral("<fonts>\n")).append(fontXMLtext).append(QStringLiteral("</fonts>\n"));

        /// format creation date
        QDate date = popplerDocument->date(QStringLiteral("CreationDate")).toUTC().date();
        if (date.isValid())
            headerText.append(DocScan::formatDate(date, creationDate));
        /// format modification date
        date = popplerDocument->date(("ModDate")).toUTC().date();
        if (date.isValid())
            headerText.append(DocScan::formatDate(date, modificationDate));

        /// retrieve author
        const QString author = popplerDocument->info(QStringLiteral("Author")).simplified();
        if (!author.isEmpty())
            headerText.append(QString(QStringLiteral("<author>%1</author>\n")).arg(DocScan::xmlify(author)));

        /// retrieve title
        QString title = popplerDocument->info(QStringLiteral("Title")).simplified();
        /// clean-up title
        if (microsoftToolRegExp.indexIn(title) == 0)
            title = microsoftToolRegExp.cap(3);
        if (!title.isEmpty())
            headerText.append(QString(QStringLiteral("<title>%1</title>\n")).arg(DocScan::xmlify(title)));

        /// retrieve subject
        const QString subject = popplerDocument->info(QStringLiteral("Subject")).simplified();
        if (!subject.isEmpty())
            headerText.append(QString(QStringLiteral("<subject>%1</subject>\n")).arg(DocScan::xmlify(subject)));

        /// retrieve keywords
        const QString keywords = popplerDocument->info(QStringLiteral("Keywords")).simplified();
        if (!keywords.isEmpty())
            headerText.append(QString(QStringLiteral("<keyword>%1</keyword>\n")).arg(DocScan::xmlify(keywords)));

        QString bodyText = QString(QStringLiteral("<body numpages=\"%1\"")).arg(numPages);
        if (textExtraction > teNone) {
            QString text;
            for (int i = 0; i < numPages; ++i)
                text += popplerDocument->page(i)->text(QRectF());
            bodyText.append(QString(QStringLiteral(" length=\"%1\"")).arg(text.length()));
            if (textExtraction >= teFullText) {
                bodyText.append(QStringLiteral(">\n"));
                if (textExtraction >= teAspell) {
                    const QString language = guessLanguage(text);
                    if (!language.isEmpty())
                        bodyText.append(QString(QStringLiteral("<language tool=\"aspell\">%1</language>\n")).arg(language));
                }
                bodyText.append(QStringLiteral("<text>")).append(DocScan::xmlify(text)).append(QStringLiteral("</text>\n"));
                bodyText.append(QStringLiteral("</body>\n"));
            } else
                bodyText.append(QStringLiteral(" />\n"));
        } else
            bodyText.append(QStringLiteral(" />\n"));
        logText.append(bodyText);

        /// look into first page for info
        if (numPages > 0) {
            Poppler::Page *page = popplerDocument->page(0);
            const QSize size = page->pageSize();
            const int mmw = size.width() * 0.3527778;
            const int mmh = size.height() * 0.3527778;
            if (mmw > 0 && mmh > 0) {
                if (page->orientation() == Poppler::Page::Seascape || page->orientation() == Poppler::Page::Landscape)
                    headerText += evaluatePaperSize(mmh, mmw);
                if (page->orientation() == Poppler::Page::Portrait || page->orientation() == Poppler::Page::UpsideDown)
                    headerText += evaluatePaperSize(mmw, mmh);
            }
        }

        if (!headerText.isEmpty())
            logText.append(QStringLiteral("<header>\n")).append(headerText).append(QStringLiteral("</header>\n"));

        delete popplerDocument;
        return true;
    } else
        return false;
}

void FileAnalyzerPDF::extractImages(QString &metaText, const QString &filename) {
    static const QString pdfimagesBinary = QStandardPaths::findExecutable(QStringLiteral("pdfimages"));
    if (pdfimagesBinary.isEmpty()) return; ///< no analysis without 'pdfimages' binary

    QProcess pdfimages(this);
    pdfimages.setWorkingDirectory(QStringLiteral("/tmp"));
    const QString prefix = QStringLiteral("docscan-pdf-extractimages-") + QString::number(qHash<QString>(filename, 0));
    const QStringList arguments = QStringList() << QStringLiteral("-j") << QStringLiteral("-jp2") << QStringLiteral("-ccitt") << QStringLiteral("-p") << QStringLiteral("-q") << filename << prefix;
    pdfimages.start(pdfimagesBinary, arguments, QIODevice::ReadOnly);
    if (!pdfimages.waitForStarted(twoMinutesInMillisec) || !pdfimages.waitForFinished(sixMinutesInMillisec))
        qWarning() << "Failed to start pdfimages for file " << filename << " and " << pdfimages.program() << pdfimages.arguments().join(' ') << " in directory " << pdfimages.workingDirectory();

    QDir dir(pdfimages.workingDirectory());
    const QStringList f = QStringList() << prefix + QStringLiteral("-*");
    const QStringList fileList = dir.entryList(f, QDir::Files, QDir::Name);
    for (const QString &imageFilename : fileList) {
        const QString absoluteImageFilename = pdfimages.workingDirectory() + QChar('/') + imageFilename;
        const QString fileExtension = imageFilename.mid(imageFilename.lastIndexOf(QChar('.')));
        if (blacklistedFileExtensions.contains(fileExtension)) {
            /// Skip this type of files
            QFile(absoluteImageFilename).remove();
            continue;
        }

        const QString mimetypeAsAttribute = QString(QStringLiteral(" mimetype=\"%1\"")).arg(DocScan::guessMimetype(imageFilename));

        emit foundEmbeddedFile(absoluteImageFilename);
        metaText.append(QStringLiteral("<embeddedfile") + mimetypeAsAttribute + QStringLiteral("><temporaryfilename>") + DocScan::xmlify(absoluteImageFilename) + QStringLiteral("</temporaryfilename></embeddedfile>"));
    }
}

void FileAnalyzerPDF::extractEmbeddedFiles(QString &metaText, Poppler::Document *popplerDocument) {
    const QList<Poppler::EmbeddedFile *> embeddedFiles = popplerDocument->embeddedFiles();
    if (!embeddedFiles.isEmpty()) {
        for (Poppler::EmbeddedFile *ef : embeddedFiles) {
            const QString fileExtension = ef->name().mid(ef->name().lastIndexOf(QChar('.')));
            if (blacklistedFileExtensions.contains(fileExtension)) {
                /// Skip this type of files
                continue;
            }

            const QString size = ef->size() >= 0 ? QString(QStringLiteral(" size=\"%1\"")).arg(ef->size()) : QString();
            const QString mimetype = ef->mimeType().isEmpty() ? DocScan::guessMimetype(ef->name()) : ef->mimeType();
            const QString mimetypeAsAttribute = QString(QStringLiteral(" mimetype=\"%1\"")).arg(mimetype);
            const QByteArray fileData = ef->data();
            const QString temporaryFilename = dataToTemporaryFile(fileData, mimetype);
            const QString embeddedFile = QStringLiteral("<embeddedfile") + size + mimetypeAsAttribute + QStringLiteral("><filename>") + DocScan::xmlify(ef->name()) + QStringLiteral("</filename>") + (ef->description().isEmpty() ? QString() : QStringLiteral("\n<description>") + DocScan::xmlify(ef->description()) + QStringLiteral("</description>")) + (temporaryFilename.isEmpty() ? QString() : QStringLiteral("<temporaryfilename>") + temporaryFilename /** no need for DocScan::xmlify */ + QStringLiteral("</temporaryfilename>")) + QStringLiteral("</embeddedfile>\n");
            metaText.append(embeddedFile);
            if (!temporaryFilename.isEmpty())
                emit foundEmbeddedFile(temporaryFilename);
        }
    }
}

void FileAnalyzerPDF::analyzeFile(const QString &filename)
{
    if (filename.endsWith(QStringLiteral(".xz")) || filename.endsWith(QStringLiteral(".gz")) || filename.endsWith(QStringLiteral(".bz2")) || filename.endsWith(QStringLiteral(".lzma"))) {
        /// File is compressed
        qWarning() << "Compressed files like " << filename << " should not directly send through this analyzer, but rather be uncompressed by FileAnalyzerMultiplexer first";
        m_isAlive = false;
        return;
    }

    m_isAlive = true;
    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    /// External programs should be both CPU and I/O 'nice'
    static const QStringList defaultArgumentsForNice = QStringList() << QStringLiteral("-n") << QStringLiteral("17") << QStringLiteral("ionice") << QStringLiteral("-c") << QStringLiteral("3");

    bool veraPDFStartedRun1 = false, veraPDFStartedRun2 = false;
    bool veraPDFIsPDFA1B = false, veraPDFIsPDFA1A = false;
    QByteArray veraPDFStandardOutputData, veraPDFStandardErrorData;
    QString veraPDFStandardOutput, veraPDFStandardError;
    long veraPDFfilesize = 0;
    int veraPDFExitCode = INT_MIN;
    QProcess veraPDF(this);
    connect(&veraPDF, &QProcess::readyReadStandardOutput, [&veraPDF, &veraPDFStandardOutputData]() {
        const QByteArray d(veraPDF.readAllStandardOutput());
        veraPDFStandardOutputData.append(d);
    });
    connect(&veraPDF, &QProcess::readyReadStandardError, [&veraPDF, &veraPDFStandardErrorData]() {
        const QByteArray d(veraPDF.readAllStandardError());
        veraPDFStandardErrorData.append(d);
    });
    if (!m_veraPDFcliTool.isEmpty()) {
        const QStringList arguments = QStringList(defaultArgumentsForNice) << m_veraPDFcliTool << QStringLiteral("-x") << QStringLiteral("-f") /** Chooses built-in Validation Profile flavour, e.g. '1b'. */ << QStringLiteral("1b") << QStringLiteral("--maxfailures") << QStringLiteral("1") << QStringLiteral("--format") << QStringLiteral("xml") << filename;
        veraPDF.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        veraPDFStartedRun1 = veraPDF.waitForStarted(twoMinutesInMillisec);
        if (!veraPDFStartedRun1)
            qWarning() << "Failed to start veraPDF for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory();
    }

    bool callasPdfAPilotStartedRun1 = false, callasPdfAPilotStartedRun2 = false;
    int callasPdfAPilotExitCode = INT_MIN;
    int callasPdfAPilotCountErrors = -1;
    int callasPdfAPilotCountWarnings = -1;
    char callasPdfAPilotPDFA1letter = '\0';
    QString callasPdfAPilotStandardOutput, callasPdfAPilotStandardError;
    QByteArray callasPdfAPilotStandardOutputData, callasPdfAPilotStandardErrorData;
    QProcess callasPdfAPilot(this);
    connect(&callasPdfAPilot, &QProcess::readyReadStandardOutput, [&callasPdfAPilot, &callasPdfAPilotStandardOutputData]() {
        const QByteArray d(callasPdfAPilot.readAllStandardOutput());
        callasPdfAPilotStandardOutputData.append(d);
    });
    connect(&callasPdfAPilot, &QProcess::readyReadStandardError, [&callasPdfAPilot, &callasPdfAPilotStandardErrorData]() {
        const QByteArray d(callasPdfAPilot.readAllStandardError());
        callasPdfAPilotStandardErrorData.append(d);
    });
    if (!m_callasPdfAPilotCLI.isEmpty()) {
        const QStringList arguments = QStringList() << defaultArgumentsForNice << m_callasPdfAPilotCLI << QStringLiteral("--quickpdfinfo") << filename;
        callasPdfAPilot.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        callasPdfAPilotStartedRun1 = callasPdfAPilot.waitForStarted(oneMinuteInMillisec);
        if (!callasPdfAPilotStartedRun1)
            qWarning() << "Failed to start callas PDF/A Pilot for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory();
    }

    QProcess *jhoveProcess = launchJHove(this, JHovePDF, filename);
    QByteArray jhoveStandardOutputData, jhoveStandardErrorData;
    connect(jhoveProcess, &QProcess::readyReadStandardOutput, [jhoveProcess, &jhoveStandardOutputData]() {
        const QByteArray d(jhoveProcess->readAllStandardOutput());
        jhoveStandardOutputData.append(d);
    });
    connect(jhoveProcess, &QProcess::readyReadStandardError, [jhoveProcess, &jhoveStandardErrorData]() {
        const QByteArray d(jhoveProcess->readAllStandardError());
        jhoveStandardErrorData.append(d);
    });
    const bool jhoveStarted = jhoveProcess != nullptr && jhoveProcess->waitForStarted(oneMinuteInMillisec);
    if (jhoveProcess != nullptr && !jhoveStarted)
        qWarning() << "Failed to start jhove for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory();

    bool pdfboxValidatorStarted = false;
    bool pdfboxValidatorValidPdf = false;
    int pdfboxValidatorExitCode = INT_MIN;
    QProcess pdfboxValidator(this);
    QByteArray pdfboxValidatorStandardOutputData, pdfboxValidatorStandardErrorData;
    QString pdfboxValidatorStandardOutput, pdfboxValidatorStandardError;
    connect(&pdfboxValidator, &QProcess::readyReadStandardOutput, [&pdfboxValidator, &pdfboxValidatorStandardOutputData]() {
        const QByteArray d(pdfboxValidator.readAllStandardOutput());
        pdfboxValidatorStandardOutputData.append(d);
    });
    connect(&pdfboxValidator, &QProcess::readyReadStandardError, [&pdfboxValidator, &pdfboxValidatorStandardErrorData]() {
        const QByteArray d(pdfboxValidator.readAllStandardError());
        pdfboxValidatorStandardErrorData.append(d);
    });
    if (!m_pdfboxValidatorJavaClass.isEmpty()) {
        static const QFileInfo fi(m_pdfboxValidatorJavaClass);
        static const QDir dir = fi.dir();
        static const QStringList jarFiles = dir.entryList(QStringList() << QStringLiteral("*.jar"), QDir::Files, QDir::Name);
        pdfboxValidator.setWorkingDirectory(dir.path());
        const QStringList arguments = QStringList(defaultArgumentsForNice) << QStringLiteral("java") << QStringLiteral("-cp") << QStringLiteral(".:") + jarFiles.join(':') << fi.fileName().remove(QStringLiteral(".class")) << QStringLiteral("--xml") << filename;
        pdfboxValidator.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
        pdfboxValidatorStarted = pdfboxValidator.waitForStarted(oneMinuteInMillisec);
        if (!pdfboxValidatorStarted)
            qWarning() << "Failed to start pdfbox Validator for file " << filename << " and " << pdfboxValidator.program() << pdfboxValidator.arguments().join(' ') << " in directory " << pdfboxValidator.workingDirectory() << ": " << QString::fromUtf8(pdfboxValidatorStandardErrorData.constData());
    }

    /// While external programs run, analyze PDF file using the Poppler library
    QString logText, metaText;
    const bool popplerWrapperOk = popplerAnalysis(filename, logText, metaText);

    if (veraPDFStartedRun1) {
        if (!veraPDF.waitForFinished(sixMinutesInMillisec))
            qWarning() << "Waiting for veraPDF failed or exceeded time limit for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory();
        veraPDFExitCode = veraPDF.exitCode();
        veraPDFStandardOutput = QString::fromUtf8(veraPDFStandardOutputData.constData()).trimmed();
        /// Sometimes veraPDF does not return complete and valid XML code. veraPDF's bug or DocScan's bug?
        if ((!veraPDFStandardOutput.contains(QStringLiteral("<rawResults>")) || !veraPDFStandardOutput.contains(QStringLiteral("</rawResults>"))) && (!veraPDFStandardOutput.contains(QStringLiteral("<ns2:cliReport")) || !veraPDFStandardOutput.contains(QStringLiteral("</ns2:cliReport>"))))
            veraPDFStandardOutput = QStringLiteral("<error>No matching opening and closing 'rawResults' or 'ns2:cliReport' tags found in output:\n") + DocScan::xmlify(veraPDFStandardOutput) + QStringLiteral("</error>");
        veraPDFStandardError = QString::fromUtf8(veraPDFStandardErrorData.constData()).trimmed();
        if (veraPDFExitCode == 0 && !veraPDFStandardOutput.isEmpty()) {
            const QString startOfOutput = veraPDFStandardOutput.left(8192);
            const int tagStart = startOfOutput.indexOf(QStringLiteral("<validationResult "));
            const int tagEnd = startOfOutput.indexOf(QStringLiteral(">"), tagStart + 10);
            const int flavourPos = startOfOutput.indexOf(QStringLiteral(" flavour=\"PDFA_1_B\""), tagStart + 10);
            const int isCompliantPos = startOfOutput.indexOf(QStringLiteral(" isCompliant=\""), tagStart + 10);
            veraPDFIsPDFA1B = tagStart > 1 && tagEnd > tagStart && flavourPos > tagStart && flavourPos < tagEnd && isCompliantPos > tagStart && isCompliantPos < tagEnd && startOfOutput.mid(isCompliantPos + 14, 4) == QStringLiteral("true");
            const int p4 = startOfOutput.indexOf(QStringLiteral("item size=\""));
            if (p4 > 1) {
                const int p5 = startOfOutput.indexOf(QStringLiteral("\""), p4 + 11);
                if (p5 > p4) {
                    bool ok = false;
                    veraPDFfilesize = startOfOutput.mid(p4 + 11, p5 - p4 - 11).toLong(&ok);
                    if (!ok) veraPDFfilesize = 0;
                }
            }

            if (veraPDFIsPDFA1B) {
                /// So, it is PDF-A/1b, then test for PDF-A/1a
                veraPDFStandardOutputData.clear(); ///< reset before launching new veraPDF process
                veraPDFStandardErrorData.clear(); ///< reset before launching new veraPDF process
                const QStringList arguments = QStringList(defaultArgumentsForNice) << m_veraPDFcliTool << QStringLiteral("-x") /** Extracts and reports PDF features. */ << QStringLiteral("-f") /** Chooses built-in Validation Profile flavour, e.g. '1b'. */ << QStringLiteral("1a") << QStringLiteral("--maxfailures") << QStringLiteral("1") << QStringLiteral("--format") << QStringLiteral("xml") << filename;
                veraPDF.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
                veraPDFStartedRun2 = veraPDF.waitForStarted(twoMinutesInMillisec);
                if (!veraPDFStartedRun2)
                    qWarning() << "Failed to start veraPDF for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory();
            } else
                qDebug() << "Skipping second run of veraPDF as file " << filename << "is not PDF/A-1b";
        } else
            qWarning() << "Execution of veraPDF failed for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory() << ": " << veraPDFStandardError;
    }

    if (callasPdfAPilotStartedRun1) {
        if (!callasPdfAPilot.waitForFinished(twoMinutesInMillisec))
            qWarning() << "Waiting for callas PDF/A Pilot failed or exceeded time limit for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory();
        callasPdfAPilotExitCode = callasPdfAPilot.exitCode();
        callasPdfAPilotStandardOutput = QString::fromUtf8(callasPdfAPilotStandardOutputData.constData()).trimmed();
        callasPdfAPilotStandardError = QString::fromUtf8(callasPdfAPilotStandardErrorData.constData()).trimmed();

        if (callasPdfAPilotExitCode == 0 && !callasPdfAPilotStandardOutput.isEmpty()) {
            static const QRegularExpression rePDFA(QStringLiteral("\\bInfo\\s+PDFA\\s+PDF/A-1([ab])"));
            const QRegularExpressionMatch match = rePDFA.match(callasPdfAPilotStandardOutput.right(512));
            callasPdfAPilotPDFA1letter = match.hasMatch() ? match.captured(1).at(0).toLatin1() : '\0';
            if (callasPdfAPilotPDFA1letter == 'a' || callasPdfAPilotPDFA1letter == 'b') {
                /// Document claims to be PDF/A-1a or PDF/A-1b, so test for errors
                callasPdfAPilotStandardOutputData.clear(); ///< reset before launching new PDF/A Pilot process
                callasPdfAPilotStandardErrorData.clear(); ///< reset before launching new PDF/A Pilot process
                const QStringList arguments = QStringList(defaultArgumentsForNice) << m_callasPdfAPilotCLI << QStringLiteral("-a") << filename;
                callasPdfAPilot.start(QStringLiteral("/usr/bin/nice"), arguments, QIODevice::ReadOnly);
                callasPdfAPilotStartedRun2 = callasPdfAPilot.waitForStarted(oneMinuteInMillisec);
                if (!callasPdfAPilotStartedRun2)
                    qWarning() << "Failed to start callas PDF/A Pilot for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory();
            } else
                qDebug() << "Skipping second run of callas PDF/A Pilot as file " << filename << "is not PDF/A-1";
        } else
            qWarning() << "Execution of callas PDF/A Pilot failed for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory() << ": " << callasPdfAPilotStandardError;
    }

    bool jhoveIsPDF = false;
    bool jhovePDFWellformed = false, jhovePDFValid = false;
    QString jhovePDFversion;
    QString jhovePDFprofile;
    int jhoveExitCode = INT_MIN;
    QString jhoveStandardOutput;
    QString jhoveStandardError;
    if (jhoveStarted) {
        if (!jhoveProcess->waitForFinished(fourMinutesInMillisec))
            qWarning() << "Waiting for jHove failed or exceeded time limit for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory();
        jhoveExitCode = jhoveProcess->exitCode();
        jhoveStandardOutput = QString::fromUtf8(jhoveStandardOutputData.constData()).replace(QLatin1Char('\n'), QStringLiteral("###"));
        jhoveStandardError = QString::fromUtf8(jhoveStandardErrorData.constData()).replace(QLatin1Char('\n'), QStringLiteral("###"));
        if (jhoveExitCode == 0 && !jhoveStandardOutput.isEmpty()) {
            jhoveIsPDF = jhoveStandardOutput.contains(QStringLiteral("Format: PDF")) && !jhoveStandardOutput.contains(QStringLiteral("ErrorMessage:"));
            static const QRegExp pdfStatusRegExp(QStringLiteral("\\bStatus: ([^#]+)"));
            if (pdfStatusRegExp.indexIn(jhoveStandardOutput) >= 0) {
                jhovePDFWellformed = pdfStatusRegExp.cap(1).startsWith(QStringLiteral("Well-Formed"), Qt::CaseInsensitive);
                jhovePDFValid = pdfStatusRegExp.cap(1).endsWith(QStringLiteral("and valid"));
            }
            static const QRegExp pdfVersionRegExp(QStringLiteral("\\bVersion: ([^#]+)#"));
            jhovePDFversion = pdfVersionRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfVersionRegExp.cap(1) : QString();
            static const QRegExp pdfProfileRegExp(QStringLiteral("\\bProfile: ([^#]+)(#|$)"));
            jhovePDFprofile = pdfProfileRegExp.indexIn(jhoveStandardOutput) >= 0 ? pdfProfileRegExp.cap(1) : QString();
        } else
            qWarning() << "Execution of jHove failed for file " << filename << " and " << jhoveProcess->program() << jhoveProcess->arguments().join(' ') << " in directory " << jhoveProcess->workingDirectory() << ": " << jhoveStandardError;
    }

    if (pdfboxValidatorStarted) {
        if (!pdfboxValidator.waitForFinished(twoMinutesInMillisec))
            qWarning() << "Waiting for pdfbox Validator failed or exceeded time limit for file " << filename << " and " << pdfboxValidator.program() << pdfboxValidator.arguments().join(' ') << " in directory " << pdfboxValidator.workingDirectory();
        pdfboxValidatorExitCode = pdfboxValidator.exitCode();
        pdfboxValidatorStandardOutput = QString::fromUtf8(pdfboxValidatorStandardOutputData.constData()).trimmed();
        pdfboxValidatorStandardError = QString::fromUtf8(pdfboxValidatorStandardErrorData.constData()).trimmed();
        if (pdfboxValidatorExitCode == 0 && !pdfboxValidatorStandardOutput.isEmpty())
            pdfboxValidatorValidPdf = pdfboxValidatorStandardOutput.contains(QStringLiteral("is a valid PDF/A-1b file"));
        else
            qWarning() << "Execution of pdfbox Validator failed for file " << filename << " and " << pdfboxValidator.program() << pdfboxValidator.arguments().join(' ') << " in directory " << pdfboxValidator.workingDirectory() << ": " << pdfboxValidatorStandardError;
    }

    if (veraPDFStartedRun2) {
        if (!veraPDF.waitForFinished(sixMinutesInMillisec))
            qWarning() << "Waiting for veraPDF failed or exceeded time limit for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory();
        veraPDFExitCode = veraPDF.exitCode();
        /// Some string magic to skip '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' from second output
        const QString newStdOut = QString::fromUtf8(veraPDFStandardOutputData.constData()).trimmed();
        const int p = newStdOut.indexOf(QStringLiteral("?>"));
        /// Sometimes veraPDF does not return complete and valid XML code. veraPDF's bug or DocScan's bug?
        if ((newStdOut.contains(QStringLiteral("<rawResults>")) && newStdOut.contains(QStringLiteral("</rawResults>"))) || (newStdOut.contains(QStringLiteral("<ns2:cliReport")) && newStdOut.contains(QStringLiteral("</ns2:cliReport>"))))
            veraPDFStandardOutput.append(QStringLiteral("\n") + (p > 1 ? newStdOut.mid(veraPDFStandardOutput.indexOf(QStringLiteral("<"), p)) : newStdOut));
        else
            veraPDFStandardOutput.append(QStringLiteral("<error>No matching opening and closing 'rawResults' or 'ns2:cliReport' tags found in output:\n") + DocScan::xmlify(newStdOut.left(512)) + QStringLiteral("</error>"));

        veraPDFStandardError = veraPDFStandardError + QStringLiteral("\n") + QString::fromUtf8(veraPDFStandardErrorData.constData()).trimmed();
        if (veraPDFExitCode == 0) {
            const QString startOfOutput = newStdOut.left(8192);
            const int tagStart = startOfOutput.indexOf(QStringLiteral("<validationResult "));
            const int tagEnd = startOfOutput.indexOf(QStringLiteral(">"), tagStart + 10);
            const int flavourPos = startOfOutput.indexOf(QStringLiteral(" flavour=\"PDFA_1_A\""), tagStart + 10);
            const int isCompliantPos = startOfOutput.indexOf(QStringLiteral(" isCompliant=\""), tagStart + 10);
            veraPDFIsPDFA1A = tagStart > 1 && tagEnd > tagStart && flavourPos > tagStart && flavourPos < tagEnd && isCompliantPos > tagStart && isCompliantPos < tagEnd && startOfOutput.mid(isCompliantPos + 14, 4) == QStringLiteral("true");
        } else
            qWarning() << "Execution of veraPDF failed for file " << filename << " and " << veraPDF.program() << veraPDF.arguments().join(' ') << " in directory " << veraPDF.workingDirectory() << ": " << veraPDFStandardError;
    }

    if (callasPdfAPilotStartedRun2) {
        if (!callasPdfAPilot.waitForFinished(fourMinutesInMillisec))
            qWarning() << "Waiting for callas PDF/A Pilot failed or exceeded time limit for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory();
        callasPdfAPilotExitCode = callasPdfAPilot.exitCode();
        callasPdfAPilotStandardOutput = callasPdfAPilotStandardOutput + QStringLiteral("\n") + QString::fromUtf8(callasPdfAPilotStandardOutputData.constData()).trimmed();
        callasPdfAPilotStandardError = callasPdfAPilotStandardError + QStringLiteral("\n") + QString::fromUtf8(callasPdfAPilotStandardErrorData.constData()).trimmed();
        if (callasPdfAPilotExitCode == 0) {
            static const QRegularExpression reSummary(QStringLiteral("\\bSummary\\t(Errors|Warnings)\\t(0|[1-9][0-9]*)\\b"));
            QRegularExpressionMatchIterator reIter = reSummary.globalMatch(callasPdfAPilotStandardOutput.right(512));
            while (reIter.hasNext()) {
                const QRegularExpressionMatch match = reIter.next();
                if (match.captured(1) == QStringLiteral("Errors")) {
                    bool ok = false;
                    callasPdfAPilotCountErrors = match.captured(2).toInt(&ok);
                    if (!ok) callasPdfAPilotCountErrors = -1;
                } else if (match.captured(1) == QStringLiteral("Warnings")) {
                    bool ok = false;
                    callasPdfAPilotCountWarnings = match.captured(2).toInt(&ok);
                    if (!ok) callasPdfAPilotCountWarnings = -1;
                }
            }
        } else
            qWarning() << "Execution of callas PDF/A Pilot failed for file " << filename << " and " << callasPdfAPilot.program() << callasPdfAPilot.arguments().join(' ') << " in directory " << callasPdfAPilot.workingDirectory() << ": " << callasPdfAPilotStandardError;
    }

    const qint64 externalProgramsEndTime = QDateTime::currentMSecsSinceEpoch();

    if (jhoveExitCode > INT_MIN) {
        /// insert data from jHove
        metaText.append(QString(QStringLiteral("<jhove exitcode=\"%1\" wellformed=\"%2\" valid=\"%3\" pdf=\"%4\"")).arg(QString::number(jhoveExitCode), jhovePDFWellformed ? QStringLiteral("yes") : QStringLiteral("no"), jhovePDFValid ? QStringLiteral("yes") : QStringLiteral("no"), jhoveIsPDF ? QStringLiteral("yes") : QStringLiteral("no")));
        if (jhovePDFversion.isEmpty() && jhovePDFprofile.isEmpty() && jhoveStandardOutput.isEmpty() && jhoveStandardError.isEmpty())
            metaText.append(QStringLiteral(" />\n"));
        else {
            metaText.append(QStringLiteral(">\n"));
            if (!jhovePDFversion.isEmpty())
                metaText.append(QString(QStringLiteral("<version>%1</version>\n")).arg(DocScan::xmlify(jhovePDFversion)));
            if (!jhovePDFprofile.isEmpty()) {
                const bool isPDFA1a = jhovePDFprofile.contains(QStringLiteral("ISO PDF/A-1, Level A"));
                const bool isPDFA1b = isPDFA1a || jhovePDFprofile.contains(QStringLiteral("ISO PDF/A-1, Level B"));
                metaText.append(QString(QStringLiteral("<profile linear=\"%2\" tagged=\"%3\" pdfa1a=\"%4\" pdfa1b=\"%5\" pdfx3=\"%6\">%1</profile>\n")).arg(DocScan::xmlify(jhovePDFprofile), jhovePDFprofile.contains(QStringLiteral("Linearized PDF")) ? QStringLiteral("yes") : QStringLiteral("no"), jhovePDFprofile.contains(QStringLiteral("Tagged PDF")) ? QStringLiteral("yes") : QStringLiteral("no"), isPDFA1a ? QStringLiteral("yes") : QStringLiteral("no"), isPDFA1b ? QStringLiteral("yes") : QStringLiteral("no"), jhovePDFprofile.contains(QStringLiteral("ISO PDF/X-3")) ? QStringLiteral("yes") : QStringLiteral("no")));
            }
            /*
            if (!jhoveStandardOutput.isEmpty())
                metaText.append(QString(QStringLiteral("<output>%1</output>\n")).arg(DocScan::xmlify(jhoveStandardOutput.replace(QStringLiteral("###"), QStringLiteral("\n")))));
            */
            if (!jhoveStandardError.isEmpty())
                metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(jhoveStandardError.replace(QStringLiteral("###"), QStringLiteral("\n")))));
            metaText.append(QStringLiteral("</jhove>\n"));
        }
    } else if (!jhoveShellscript.isEmpty())
        metaText.append(QStringLiteral("<jhove><error>jHove failed to start or was never started</error></jhove>\n"));
    else
        metaText.append(QStringLiteral("<jhove><info>jHove not configured to run</info></jhove>\n"));

    if (veraPDFExitCode > INT_MIN) {
        /// insert XML data from veraPDF
        metaText.append(QString(QStringLiteral("<verapdf exitcode=\"%1\" filesize=\"%2\" pdfa1b=\"%3\" pdfa1a=\"%4\">\n")).arg(QString::number(veraPDFExitCode), QString::number(veraPDFfilesize), veraPDFIsPDFA1B ? QStringLiteral("yes") : QStringLiteral("no"), veraPDFIsPDFA1A ? QStringLiteral("yes") : QStringLiteral("no")));
        if (!veraPDFStandardOutput.isEmpty()) {
            /// Check for and omit XML header if it exists
            const int p = veraPDFStandardOutput.indexOf(QStringLiteral("?>"));
            metaText.append(p > 1 ? veraPDFStandardOutput.mid(veraPDFStandardOutput.indexOf(QStringLiteral("<"), p)) : veraPDFStandardOutput);
        } else if (!veraPDFStandardError.isEmpty())
            metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(veraPDFStandardError)));
        metaText.append(QStringLiteral("</verapdf>\n"));
    } else if (!m_veraPDFcliTool.isEmpty())
        metaText.append(QStringLiteral("<verapdf><error>veraPDF failed to start or was never started</error></verapdf>\n"));
    else
        metaText.append(QStringLiteral("<verapdf><info>veraPDF not configured to run</info></verapdf>\n"));

    if (pdfboxValidatorExitCode > INT_MIN) {
        /// insert result from Apache's PDFBox
        metaText.append(QString(QStringLiteral("<pdfboxvalidator exitcode=\"%1\" pdfa1b=\"%2\">\n")).arg(QString::number(pdfboxValidatorExitCode), pdfboxValidatorValidPdf ? QStringLiteral("yes") : QStringLiteral("no")));
        if (!pdfboxValidatorStandardOutput.isEmpty()) {
            if (!pdfboxValidatorStandardOutput.startsWith(QChar('<')) || !pdfboxValidatorStandardOutput.endsWith(QChar('>'))) {
                /// Output does not look like XML output or is capped.
                /// Treat like plain text.
                metaText.append(DocScan::xmlify(pdfboxValidatorStandardOutput));
            } else
                metaText.append(pdfboxValidatorStandardOutput);
        } else if (!pdfboxValidatorStandardError.isEmpty())
            metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(pdfboxValidatorStandardError)));
        metaText.append(QStringLiteral("</pdfboxvalidator>\n"));
    } else if (!m_pdfboxValidatorJavaClass.isEmpty())
        metaText.append(QStringLiteral("<pdfboxvalidator><error>pdfbox Validator failed to start or was never started</error></pdfboxvalidator>\n"));
    else
        metaText.append(QStringLiteral("<pdfboxvalidator><info>pdfbox Validator not configured to run</info></pdfboxvalidator>\n"));

    if (callasPdfAPilotExitCode > INT_MIN) {
        const bool isPDFA1a = callasPdfAPilotPDFA1letter == 'a' && callasPdfAPilotCountErrors == 0 && callasPdfAPilotCountWarnings == 0;
        const bool isPDFA1b = isPDFA1a || (callasPdfAPilotPDFA1letter == 'b' && callasPdfAPilotCountErrors == 0 && callasPdfAPilotCountWarnings == 0);
        metaText.append(QString(QStringLiteral("<callaspdfapilot exitcode=\"%1\" pdfa1b=\"%2\" pdfa1a=\"%3\">\n")).arg(QString::number(callasPdfAPilotExitCode), isPDFA1b ? QStringLiteral("yes") : QStringLiteral("no"), isPDFA1a ? QStringLiteral("yes") : QStringLiteral("no")));
        if (!callasPdfAPilotStandardOutput.isEmpty())
            metaText.append(DocScan::xmlify(callasPdfAPilotStandardOutput));
        else if (!callasPdfAPilotStandardError.isEmpty())
            metaText.append(QString(QStringLiteral("<error>%1</error>\n")).arg(DocScan::xmlify(callasPdfAPilotStandardError)));
        metaText.append(QStringLiteral("</callaspdfapilot>"));
    } else if (!m_callasPdfAPilotCLI.isEmpty())
        metaText.append(QStringLiteral("<callaspdfapilot><error>callas PDF/A Pilot failed to start or was never started</error></callaspdfapilot>\n"));
    else
        metaText.append(QStringLiteral("<callaspdfapilot><info>callas PDF/A Pilot not configured to run</info></callaspdfapilot>\n"));

    /// file information including size
    const QFileInfo fi = QFileInfo(filename);
    metaText.append(QString(QStringLiteral("<file size=\"%1\" />\n")).arg(fi.size()));

    if (!metaText.isEmpty())
        logText.append(QStringLiteral("<meta>\n")).append(metaText).append(QStringLiteral("</meta>\n"));
    const qint64 endTime = QDateTime::currentMSecsSinceEpoch();

    logText.prepend(QString(QStringLiteral("<fileanalysis filename=\"%1\" status=\"ok\" time=\"%2\" external_time=\"%3\">\n")).arg(DocScan::xmlify(filename), QString::number(endTime - startTime), QString::number(externalProgramsEndTime - startTime)));
    logText += QStringLiteral("</fileanalysis>\n");

    if (popplerWrapperOk || jhoveIsPDF || pdfboxValidatorValidPdf)
        /// At least one tool thought the file was ok
        emit analysisReport(objectName(), logText);
    else
        /// No tool could handle this file, so give error message
        emit analysisReport(objectName(), QString(QStringLiteral("<fileanalysis filename=\"%1\" message=\"invalid-fileformat\" status=\"error\" external_time=\"%2\"><meta><file size=\"%3\" /></meta></fileanalysis>\n")).arg(filename, QString::number(externalProgramsEndTime - startTime)).arg(fi.size()));

    m_isAlive = false;
}

const QStringList FileAnalyzerPDF::blacklistedFileExtensions = QStringList() << QStringLiteral(".pbm") << QStringLiteral(".ppm") << QStringLiteral(".ccitt") << QStringLiteral(".params") << QStringLiteral(".joboptions");
