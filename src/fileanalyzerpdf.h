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

#ifndef FILEANALYZERPDF_H
#define FILEANALYZERPDF_H

#include <QObject>
#include <QTemporaryDir>

#include <poppler-qt5.h>

#include "fileanalyzerabstract.h"
#include "jhovewrapper.h"

/**
 * Analyzing code for Portable Document File documents.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerPDF : public FileAnalyzerAbstract, public JHoveWrapper
{
    Q_OBJECT

public:
    enum XMPPDFConformance {xmpError = -1, xmpNone = 0, xmpPDFA1b = 10, xmpPDFA1a = 11, xmpPDFA2b = 20, xmpPDFA2a = 21, xmpPDFA2u = 22, xmpPDFA3b = 30, xmpPDFA3a = 31, xmpPDFA3u = 32, xmpPDFA4 = 40};

    explicit FileAnalyzerPDF(QObject *parent = nullptr);

    virtual bool isAlive() override;

    void setupJhove(const QString &shellscript);
    void setupVeraPDF(const QString &cliTool);
    void setupPdfBoXValidator(const QString &pdfboxValidatorJavaClass);
    void setupCallasPdfAPilotCLI(const QString &callasPdfAPilotCLI);
    void setupAdobePreflightReportDirectory(const QString &adobePreflightReportDirectory);
    void setupQoppaJPDFPreflightDirectory(const QString &qoppaJPDFPreflightDirectory);
    void setupThreeHeightsValidatorShellCLI(const QString &threeHeightsValidatorShellCLI, const QString &threeHeightsValidatorLicenseKey);

    /**
     * A PDF file to be analyzed may be known under a different name.
     * For example, '/home/user/dissertation851986.pdf.xz' may be have been
     * uncompressed to a temporary location under the name '/tmp/0gH3yA.pdf'
     * during this analysis.
     * Invoking  setAliasName("/tmp/0gH3yA.pdf", "/home/user/dissertation851986.pdf.xz")
     * established the relation between both files.
     */
    void setAliasName(const QString &toAnalyzeFilename, const QString &aliasFilename);

    void setPDFAValidationOptions(const bool validateOnlyPDFAfiles, const bool downgradeToPDFA1b, const XMPPDFConformance enforcedValidationLevel);

public slots:
    virtual void analyzeFile(const QString &filename) override;

private:
    struct ExtendedFontInfo {
        QString name, typeName, fileName;
        bool isEmbedded, isSubset;
        int firstPageNumber, lastPageNumber;
        QSet<int> pageNumbers;

        explicit ExtendedFontInfo()
            : firstPageNumber(INT_MAX), lastPageNumber(INT_MIN) {

        }

        explicit ExtendedFontInfo(const Poppler::FontInfo &fi, const int pageNumber)
            : name(fi.name()), typeName(fi.typeName()), fileName(fi.file()), isEmbedded(fi.isEmbedded()), isSubset(fi.isSubset()),
              firstPageNumber(pageNumber), lastPageNumber(pageNumber)
        {
            pageNumbers.insert(pageNumber);
        }

        void recordOccurrence(int pageNumber) {
            if (pageNumber < firstPageNumber) firstPageNumber = pageNumber;
            if (pageNumber > lastPageNumber) lastPageNumber = pageNumber;
            pageNumbers.insert(pageNumber);
        }

        bool isValid() const {
            return lastPageNumber > INT_MIN && firstPageNumber < INT_MAX && firstPageNumber <= lastPageNumber;
        }
    };

    bool m_isAlive;
    QString m_veraPDFcliTool;
    QString m_pdfboxValidatorJavaClass;
    QString m_callasPdfAPilotCLI;
    QString m_adobePreflightReportDirectory;
    QString m_qoppaJPDFPreflightDirectory;
    QString m_threeHeightsValidatorShellCLI, m_threeHeightsValidatorLicenseKey;
    QString m_toAnalyzeFilename, m_aliasFilename;
    bool m_validateOnlyPDFAfiles, m_downgradeToPDFA1b;
    XMPPDFConformance m_enforcedValidationLevel;
    QTemporaryDir m_tempDirDowngradeToPDFA1b;

    static const QStringList blacklistedFileExtensions;

    enum PDFVersion { pdfVersionError = -1, pdfVersion1dot1 = 11, pdfVersion1dot2 = 12, pdfVersion1dot3 = 13, pdfVersion1dot4 = 14, pdfVersion1dot5 = 15, pdfVersion1dot6 = 16, pdfVersion1dot7 = 17, pdfVersion2dot0 = 20, pdfVersion2dot1 = 21, pdfVersion2dot2 = 22};

    bool popplerAnalysis(const QString &filename, QString &logText, QString &metaText);
    PDFVersion pdfVersionAnalysis(const QString &filename);
    inline QString pdfVersionToString(const PDFVersion pdfVersion) const;
    XMPPDFConformance xmpAnalysis(const QString &filename, const PDFVersion pdfVersion, QString &metaText);
    inline QString xmpPDFConformanceToString(const XMPPDFConformance xmpPDFConformance) const;

    /**
     * Check if PDF version matches the XMP PDF/A conformance level and part.
     * For example, a file claiming to be PDF/A-1a must have PDF version 1.4.
     *
     * @param pdfVersion any valid value of PDFVersion
     * @param xmpPDFConformance any valid value of XMPPDFConformance
     * @return true if PDF version and XMP data match
     */
    inline bool pdfVersionMatchesXMPconformance(const FileAnalyzerPDF::PDFVersion pdfVersion, const FileAnalyzerPDF::XMPPDFConformance xmpPDFConformance);

    bool downgradingPDFA(const QString &filename);
    bool adobePreflightReportAnalysis(const QString &filename, QString &metaText);
    void extractImages(QString &metaText, const QString &filename);
    void extractEmbeddedFiles(QString &metaText, Poppler::Document *popplerDocument);

protected slots:
    void delayedToolcheck() override;
};

#endif // FILEANALYZERPDF_H
