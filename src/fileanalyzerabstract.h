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

#ifndef FILEANALYZERABSTRACT_H
#define FILEANALYZERABSTRACT_H

#include <QObject>
#include <QHash>
#include <QSet>

#include "watchable.h"

class QDate;

/**
 * Common class for file analyzing classes.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerAbstract : public QObject, public Watchable
{
    Q_OBJECT
public:
    enum TextExtraction {teNone = 0, teLength = 5, teFullText = 10, teAspell = 15};

    static const QString licenseCategoryProprietary, licenseCategoryFreeware, licenseCategoryOpen;

    explicit FileAnalyzerAbstract(QObject *parent = nullptr);

    virtual void setTextExtraction(TextExtraction textExtraction);

signals:
    /**
     * Reporting findings of analysis
     */
    void analysisReport(QString, QString);

public slots:
    /**
     * Requests analyzer object to analyze file.
     * Analysis may be asynchronous and will set this object alive.
     * Has to be implemented by every class to be instanciated
     * and inheriting from this class.
     *
     * @param filename file to analyze
     */
    virtual void analyzeFile(const QString &filename) = 0;

    /**
     * Schedule a file for later analysis. Being a temporary file,
     * erase this file after the analysis.
     *
     * @param filename filename for later analysis, to be deleted afterwards
     */
    void analyzeTemporaryFile(const QString &filename);

protected:
    static const QString creationDate, modificationDate;
    static const QRegExp microsoftToolRegExp;

    TextExtraction textExtraction;

    QString guessLanguage(const QString &text) const;
    QStringList runAspell(const QString &text, const QString &dictionary) const;
    QString guessTool(const QString &toolString, const QString &altToolString = QString()) const;
    QString formatDate(const QDate date, const QString &base = QString()) const;
    QString evaluatePaperSize(int mmw, int mmh) const;
    QString dataToTemporaryFile(const QByteArray &data, const QString &mimetype);

private:
    static QSet<QString> aspellLanguages;

    QSet<QString> getAspellLanguages() const;
};

#endif // FILEANALYZERABSTRACT_H
