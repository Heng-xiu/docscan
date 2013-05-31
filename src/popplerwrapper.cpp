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


    Major parts of this source code file are derived from or inspired by
    Poppler, an open source PDF library released under the GNU General
    Public License version 2 or later.
    Modifications and additions to this source code were made by
    Thomas Fischer <thomas.fischer@his.se>

 */

#include "poppler/OutputDev.h"

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-font.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/GfxState.h>

#include "PDFDoc.h"
#include "PDFDocFactory.h"

#include <QStringList>
#include <QDebug>

#include "general.h"
#include "popplerwrapper.h"



class ImageInfoOutputDev: public OutputDev
{
private:
    QString logText;
    int currentPage;
    poppler::document *m_document;

public:
    ImageInfoOutputDev(poppler::document *document)
        : currentPage(0), m_document(document) {
        /// nothing
    }


    void clear() {
        logText.clear();
    }

    QString getLogText() const {
        return logText;
    }

    // Start a page.
    void startPage(int pageNum, GfxState */*state*/) {
        if (pageNum <= 10) {
            logText.append(QString(QLatin1String("<page number=\"%1\">\n")).arg(pageNum));
            currentPage = pageNum;
        } else
            currentPage = -1;
    }

    // End a page.
    virtual void endPage() {
        if (currentPage >= 0) {
            poppler::page *page = m_document->create_page(currentPage - 1);
            if (page != NULL) {
                const QString cookedText = page != NULL ? DocScan::xmlify(QString::fromUtf8(page->text().to_utf8().data()).simplified()) : QString();
                if (!cookedText.isEmpty())
                    logText.append(QString(QLatin1String("<text length=\"%1\">")).arg(page->text().length())).append(cookedText).append(QLatin1String("</text>\n"));
                else
                    logText.append(QString(QLatin1String("<text length=\"%1\" />\n")).arg(page->text().length()));
            }

            logText.append(QLatin1String("</page>\n"));
        }
        currentPage = -1;
    }

    void listImage(GfxState */*state*/, Object */*ref*/, Stream *str,
                   int width, int height,
                   GfxImageColorMap *colorMap,
                   GBool /*interpolate*/, GBool /*inlineImg*/) {
        if (currentPage >= 0) {
            DocScan::XMLNode node;
            node.name = QLatin1String("img");

            switch (str->getKind()) {
            case strCCITTFax:
                node.attributes.insert(QLatin1String("type"), QLatin1String("ccitt"));
                break;
            case strDCT:
                node.attributes.insert(QLatin1String("type"), QLatin1String("jpeg"));
                break;
            case strJPX:
                node.attributes.insert(QLatin1String("type"), QLatin1String("jpx"));
                break;
            case strJBIG2:
                node.attributes.insert(QLatin1String("type"), QLatin1String("jbig2"));
                break;
            default: {
                /// nothing
            }
            }

            node.attributes.insert(QLatin1String("width"), QString::number(width));
            node.attributes.insert(QLatin1String("height"), QString::number(height));
            if (colorMap != NULL)
                node.attributes.insert(QLatin1String("bits"), QString::number(colorMap->getBits()));

            logText.append(DocScan::xmlNodeToText(node));
        }
    }

    GBool tilingPatternFill(GfxState */*state*/, Gfx */*gfx*/, Catalog */*cat*/, Object */*str*/,
                            double */*pmat*/, int /*paintType*/, int /*tilingType*/, Dict */*resDict*/,
                            double */*mat*/, double */*bbox*/,
                            int /*x0*/, int /*y0*/, int /*x1*/, int /*y1*/,
                            double /*xStep*/, double /*yStep*/) {
        return gTrue;
        // do nothing -- this avoids the potentially slow loop in Gfx.cc
    }

    void drawImageMask(GfxState *state, Object *ref, Stream *str,
                       int width, int height, GBool /*invert*/,
                       GBool interpolate, GBool inlineImg) {
        listImage(state, ref, str, width, height, NULL, interpolate, inlineImg);
    }

    void drawImage(GfxState *state, Object *ref, Stream *str,
                   int width, int height,
                   GfxImageColorMap *colorMap,
                   GBool interpolate, int */*maskColors*/, GBool inlineImg) {
        listImage(state, ref, str, width, height, colorMap, interpolate, inlineImg);
    }

    void drawMaskedImage(
        GfxState *state, Object *ref, Stream *str,
        int width, int height, GfxImageColorMap *colorMap, GBool interpolate,
        Stream */*maskStr*/, int maskWidth, int maskHeight, GBool /*maskInvert*/, GBool maskInterpolate) {
        listImage(state, ref, str, width, height, colorMap, interpolate, gFalse);
        listImage(state, ref, str, maskWidth, maskHeight, NULL, maskInterpolate, gFalse);
    }

    void drawSoftMaskedImage(
        GfxState *state, Object *ref, Stream *str,
        int width, int height, GfxImageColorMap *colorMap, GBool interpolate,
        Stream *maskStr, int maskWidth, int maskHeight,
        GfxImageColorMap *maskColorMap, GBool maskInterpolate) {
        listImage(state, ref, str, width, height, colorMap, interpolate, gFalse);
        listImage(state, ref, maskStr, maskWidth, maskHeight, maskColorMap, maskInterpolate, gFalse);
    }

    // Does this device use upside-down coordinates?
    // (Upside-down means (0,0) is the top left corner of the page.)
    virtual GBool upsideDown() {
        return gTrue;
    }

    // Does this device use drawChar() or drawString()?
    virtual GBool useDrawChar() {
        return gFalse;
    }

    // Does this device use beginType3Char/endType3Char?  Otherwise,
    // text in Type 3 fonts will be drawn with drawChar/drawString.
    virtual GBool interpretType3Chars() {
        return gFalse;
    }
};



PopplerWrapper *PopplerWrapper::createPopplerWrapper(const QString &filename)
{
    poppler::document *document = poppler::document::load_from_file(filename.toStdString());
    if (document == NULL)
        return NULL;

    return new PopplerWrapper(document, filename);
}

PopplerWrapper::PopplerWrapper(poppler::document *document, const QString &filename)
    : m_document(document), m_filename(filename)
{
    Q_ASSERT(document != NULL);
}

PopplerWrapper::~PopplerWrapper()
{
    delete m_document;
}

void PopplerWrapper::getPdfVersion(int &majorVersion, int &minorVersion) const
{
    majorVersion = minorVersion = 0;
    m_document->get_pdf_version(&majorVersion, &minorVersion);
}

QStringList PopplerWrapper::fontNames() const
{
    QStringList result;
    poppler::font_iterator *it = m_document->create_font_iterator();
    while (it->has_next()) {
        std::vector<poppler::font_info> fonts = it->next();
        // do domething with the fonts
        for (std::vector<poppler::font_info>::const_iterator fit = fonts.begin(); fit != fonts.end(); ++fit) {
            QString fontType = "???";
            poppler::font_info fi = *fit;

            /* FIXME
            switch ((poppler::font_info::type_enum)fi.type){
            case poppler::font_info::unknown:
                fontType= QLatin1String("unknown");
                break;
            case poppler::font_info::type1:
                fontType= QLatin1String("Type 1");
                break;
            case poppler::font_info::type1c:
                fontType= QLatin1String("Type 1C");
                break;
            case poppler::font_info::type3:
                fontType= QLatin1String("Type 3");
                break;
            case poppler::font_info::truetype:
                fontType= QLatin1String("TrueType");
                break;
            case poppler::font_info::cid_type0:
                fontType= QLatin1String("CID Type 0");
                break;
            case poppler::font_info::cid_type0c:
                fontType= QLatin1String("CID Type 0C");
                break;
            case poppler::font_info::cid_truetype:
                fontType= QLatin1String("CID TrueType");
                break;
            case poppler::font_info::type1c_ot:
                fontType= QLatin1String("Type 1C (OpenType)");
                break;
            case poppler::font_info::truetype_ot:
                fontType= QLatin1String("TrueType (OpenType)");
                break;
            case poppler::font_info::cid_type0c_ot:
                fontType= QLatin1String("CID Type 0C (OpenType)");
                break;
            case poppler::font_info::cid_truetype_ot:
                fontType= QLatin1String("CID TrueType (OpenType)");
                break;
            }
            */

            result << QString::fromStdString(fi.name()) + QLatin1Char('|') + fontType;
        }
    }
    // after we are done with the iterator, it must be deleted
    delete it;

    return result;
}

QString PopplerWrapper::info(const QString &field) const
{
    return QString::fromStdString(m_document->info_key(field.toStdString()).to_latin1());
}

QDateTime PopplerWrapper::date(const QString &field) const
{
    return QDateTime::fromTime_t(m_document->info_date(field.toStdString()));
}

int PopplerWrapper::numPages() const
{
    return m_document->pages();
}

QString PopplerWrapper::plainText() const
{
    QString result;
    for (int i = 0; i < numPages() && result.length() < 16384; ++i) {
        poppler::page *page = m_document->create_page(i);
        result.append(QString::fromStdString(page->text().to_latin1()));
    }
    return result;
}

QSizeF PopplerWrapper::pageSize() const
{
    if (numPages() < 1)
        return QSize(0, 0);

    poppler::page *page = m_document->create_page(0);
    poppler::rectf rect = page->page_rect();
    if (page->orientation() == poppler::page::seascape || page->orientation() == poppler::page::landscape)
        return QSizeF(rect.height(), rect.width());
    else
        return QSizeF(rect.width(), rect.height());
}

bool PopplerWrapper::isLocked() const
{
    return m_document->is_locked();
}

bool PopplerWrapper::isEncrypted() const
{
    return m_document->is_encrypted();
}

QString PopplerWrapper::imagesLog()
{
    ImageInfoOutputDev iiod(m_document);

    GooString fileName(m_filename.toAscii().data());
    PDFDoc *doc = PDFDocFactory().createPDFDoc(fileName);

    doc->displayPages(&iiod, 0, numPages(), 72, 72, 0, gTrue, gFalse, gFalse);

    delete doc;

    return iiod.getLogText();
}
