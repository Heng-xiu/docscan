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

#include <limits>

#include <QProcess>

#include "fileanalyzerabstract.h"

FileAnalyzerAbstract::FileAnalyzerAbstract(QObject *parent)
    : QObject(parent)
{
}

QString FileAnalyzerAbstract::evaluatePaperSize(int mmw, int mmh)
{
    QString result;
    result = QString("<pagesize width=\"%1\" height=\"%2\" unit=\"mm\" />\n").arg(mmw).arg(mmh);

    if (mmw >= 208 && mmw <= 212 && mmh >= 295 && mmh <= 299)
        result += QString("<pagesize name=\"A4\" orientation=\"portrait\" unit=\"name\" />\n");
    else if (mmh >= 208 && mmh <= 212 && mmw >= 295 && mmw <= 299)
        result += QString("<pagesize name=\"A4\" orientation=\"landscape\" unit=\"name\" />\n");
    else if (mmw >= 214 && mmw <= 218 && mmh >= 277 && mmh <= 281)
        result += QString("<pagesize name=\"Letter\" orientation=\"portrait\" unit=\"name\" />\n");
    else if (mmw >= 214 && mmw <= 218 && mmh >= 254 && mmh <= 258)
        result += QString("<pagesize name=\"Legal\" orientation=\"portrait\" unit=\"name\" />\n");

    return result;
}

QStringList FileAnalyzerAbstract::runAspell(const QString &text, const QString &dictionary)
{
    QStringList wordList;
    QProcess aspell(this);
    QStringList args = QStringList() << "-d" << dictionary << "list";
    aspell.start("/usr/bin/aspell", args);
    if (aspell.waitForStarted(10000)) {
        aspell.write(text.toUtf8());
        aspell.closeWriteChannel();
        while (aspell.waitForReadyRead(10000)) {
            while (aspell.canReadLine()) {
                wordList << aspell.readLine();
            }
        }
        if (!aspell.waitForFinished(10000))
            aspell.kill();
    }
    return wordList;
}

QString FileAnalyzerAbstract::guessLanguage(const QString &text)
{
    int count = std::numeric_limits<int>::max();
    QString best = QString::null;

    foreach(QString lang, getAspellLanguages()) {
        int c = runAspell(text, lang).count();
        if (c < count) {
            count = c;
            best = lang;
        }
    }

    return best;
}

QStringList FileAnalyzerAbstract::getAspellLanguages()
{
    if (aspellLanguages.isEmpty()) {
        /*
        QRegExp language("^[ ]+([a-z]{2})( |$)");
        QProcess aspell(this);
        QStringList args = QStringList() << "--help";
        aspell.start("/usr/bin/aspell", args);
        if (aspell.waitForStarted(10000)) {
            aspell.closeWriteChannel();
            while (aspell.waitForReadyRead(10000)) {
                while (aspell.canReadLine()) {
                    if (language.indexIn(aspell.readLine()) >= 0) {
                        aspellLanguages << language.cap(1);
                    }
                }
            }
            if (!aspell.waitForFinished(10000))
                aspell.kill();
        }
        */
        aspellLanguages << "de" << "en" << "sv" << "fi" << "fr";
    }
    return aspellLanguages;
}

QString FileAnalyzerAbstract::guessLicenseFromProduct(const QString &product)
{
    const QString lowerText = product.toLower();

    if (lowerText.contains("microsoft") || lowerText.contains("adobe") || lowerText.contains("excel") || lowerText.contains("powerpoint"))
        return QLatin1String("proprietary");
    if (lowerText.contains("neooffice") || lowerText.contains("broffice") || lowerText.contains("koffice") || lowerText.contains("calligra"))
        return QLatin1String("opensource");
    if (lowerText.contains("openoffice") || lowerText.contains("libreoffice"))
        return QLatin1String("opensource|LGPL");
    if (lowerText.contains("pdftex") || lowerText.contains("ghostscript") || lowerText.contains("pdfcreator"))
        return QLatin1String("opensource|GPL");

    return QLatin1String("unknown");
}

QStringList FileAnalyzerAbstract::aspellLanguages;
