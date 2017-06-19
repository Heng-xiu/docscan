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

#include "fileanalyzerabstract.h"

#include <limits>

#include <QProcess>
#include <QCoreApplication>
#include <QDate>
#include <QTextStream>
#include <QFile>
#include <QCryptographicHash>

#include "guessing.h"
#include "general.h"

FileAnalyzerAbstract::FileAnalyzerAbstract(QObject *parent)
    : QObject(parent), textExtraction(teNone)
{
    setObjectName(QString(QLatin1String(metaObject()->className())).toLower());

    const QSet<QString> stringSet = getAspellLanguages();
    QString allLanguages;
    for (const QString &lang : stringSet) {
        if (!allLanguages.isEmpty())
            allLanguages.append(QStringLiteral(","));
        allLanguages.append(lang);
    }
    // FIXME reporting does not work yet, as signals are not yet set up
    emit analysisReport(objectName(), QString(QStringLiteral("<initialization source=\"aspell\" type=\"languages\">%1</initialization>\n").arg(allLanguages)));
}

void FileAnalyzerAbstract::setTextExtraction(TextExtraction textExtraction) {
    this->textExtraction = textExtraction;
}

QStringList FileAnalyzerAbstract::runAspell(const QString &text, const QString &dictionary) const
{
    QStringList wordList;
    QProcess aspell(QCoreApplication::instance());
    const QStringList args = QStringList() << QStringLiteral("-d") << dictionary << QStringLiteral("list");
    aspell.start(QStringLiteral("/usr/bin/aspell"), args);
    if (aspell.waitForStarted(10000)) {
        const qint64 bytesWritten = aspell.write(text.toUtf8());
        if (bytesWritten < 0) return QStringList(); /// something went wrong
        aspell.closeWriteChannel();

        QTextStream ts(&aspell);
        ts.setCodec("UTF-8");
        while (aspell.waitForReadyRead(10000)) {
            QString line;
            while (!(line = ts.readLine()).isNull()) {
                wordList << line;
            }
        }
        if (!aspell.waitForFinished(10000))
            aspell.kill();
    }
    return wordList;
}

QString FileAnalyzerAbstract::guessLanguage(const QString &text) const
{
    int count = std::numeric_limits<int>::max();
    QString best;

    static const QSet<QString> langs = getAspellLanguages();
    for (const QString &lang : langs) {
        int c = runAspell(text, lang).count();
        if (c > 0 && c < count) { /// if c==0, no misspelled words where found, likely due to an error
            count = c;
            best = lang;
        }
    }

    return best;
}

QSet<QString> FileAnalyzerAbstract::getAspellLanguages() const
{
    if (aspellLanguages.isEmpty()) {
        QRegExp language(QStringLiteral("^[a-z]{2}(_[A-Z]{2})?$"));
        QProcess aspell(qApp);
        const QStringList args = QStringList() << QStringLiteral("dicts");
        aspell.start(QStringLiteral("/usr/bin/aspell"), args);
        if (aspell.waitForStarted(10000)) {
            aspell.closeWriteChannel();
            while (aspell.waitForReadyRead(10000)) {
                while (aspell.canReadLine()) {
                    const QString line = aspell.readLine().simplified();
                    if (language.indexIn(line) >= 0) {
                        aspellLanguages.insert(language.cap(0));
                    }
                }
            }
            if (!aspell.waitForFinished(10000))
                aspell.kill();
        }
    }

    return aspellLanguages;
}

QString FileAnalyzerAbstract::guessTool(const QString &toolString, const QString &altToolString) const
{
    QString result;
    QString text;

    if (microsoftToolRegExp.indexIn(altToolString) == 0)
        text = microsoftToolRegExp.cap(1);
    else if (!toolString.isEmpty())
        text = toolString;
    else if (!altToolString.isEmpty())
        text = altToolString;

    if (!text.isEmpty())
        result += Guessing::programToXML(text);

    return result;
}

QString FileAnalyzerAbstract::evaluatePaperSize(int mmw, int mmh) const
{
    QString formatName;

    if (mmw >= 208 && mmw <= 212 && mmh >= 295 && mmh <= 299)
        formatName = QStringLiteral("A4");
    else if (mmh >= 208 && mmh <= 212 && mmw >= 295 && mmw <= 299)
        formatName = QStringLiteral("A4");
    else if (mmw >= 214 && mmw <= 218 && mmh >= 277 && mmh <= 281)
        formatName = QStringLiteral("Letter");
    else if (mmh >= 214 && mmh <= 218 && mmw >= 277 && mmw <= 281)
        formatName = QStringLiteral("Letter");
    else if (mmw >= 214 && mmw <= 218 && mmh >= 254 && mmh <= 258)
        formatName = QStringLiteral("Legal");
    else if (mmh >= 214 && mmh <= 218 && mmw >= 254 && mmw <= 258)
        formatName = QStringLiteral("Legal");

    return formatName.isEmpty()
           ? QString(QStringLiteral("<papersize height=\"%1\" width=\"%2\" orientation=\"%3\" />\n")).arg(QString::number(mmh), QString::number(mmw), mmw > mmh ? QStringLiteral("landscape") : QStringLiteral("portrait"))
           : QString(QStringLiteral("<papersize height=\"%1\" width=\"%2\" orientation=\"%4\">%3</papersize>\n")).arg(QString::number(mmh), QString::number(mmw), formatName, mmw > mmh ? QStringLiteral("landscape") : QStringLiteral("portrait"));
}

QString FileAnalyzerAbstract::dataToTemporaryFile(const QByteArray &data, const QString &mimetype) {
    static QCryptographicHash hash(QCryptographicHash::Md5);
    hash.reset(); ///< Reset neccessary as hash object is static
    hash.addData(data);
    const QString temporaryFilename = QStringLiteral("/tmp/docscan-embeddedfile-") + hash.result().toHex() + DocScan::extensionForMimetype(mimetype);
    QFile temporaryFile(temporaryFilename);
    if (temporaryFile.open(QFile::WriteOnly)) {
        temporaryFile.write(data);
        temporaryFile.close();
        return temporaryFilename;
    } else
        return QString();
}

QSet<QString> FileAnalyzerAbstract::aspellLanguages;

const QRegExp FileAnalyzerAbstract::microsoftToolRegExp(QStringLiteral("^(Microsoft\\s(.+\\S) [ -][ ]?(\\S.*)$"));
const QString FileAnalyzerAbstract::creationDate = QStringLiteral("creation");
const QString FileAnalyzerAbstract::modificationDate = QStringLiteral("modification");
