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

#ifndef FILEANALYZERABSTRACT_H
#define FILEANALYZERABSTRACT_H

#include <QObject>
#include <QMap>

#include "watchable.h"

class QDate;

/**
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class FileAnalyzerAbstract : public QObject, public Watchable
{
    Q_OBJECT
public:
    static const QString licenseCategoryProprietary, licenseCategoryFreeware, licenseCategoryOpen;

    explicit FileAnalyzerAbstract(QObject *parent = 0);

signals:
    void analysisReport(QString);

public slots:
    virtual void analyzeFile(const QString &filename) = 0;

protected:
    static const QString creationDate, modificationDate;
    static const QRegExp microsoftToolRegExp;

    QString guessLanguage(const QString &text) const;
    QStringList runAspell(const QString &text, const QString &dictionary) const;
    QMap<QString, QString> guessLicense(const QString &license) const;
    QMap<QString, QString> guessOpSys(const QString &opsys) const;
    QMap<QString, QString> guessProgram(const QString &program) const;
    QString guessTool(const QString &toolString, const QString &altToolString = QString::null) const;
    QString guessFont(const QString &fontString, const QString &typeName = QString::null) const;
    QStringList guessLicenseFromFont(const QString &fontName) const;
    QString formatDate(const QDate &date, const QString &base = QString::null) const;
    QString evaluatePaperSize(int mmw, int mmh) const;

    QString formatMap(const QString &key, const QMap<QString, QString> &attrs) const;

private:
    static QStringList aspellLanguages;

    QStringList getAspellLanguages() const;
};

#endif // FILEANALYZERABSTRACT_H
