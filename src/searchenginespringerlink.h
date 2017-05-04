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

#ifndef SEARCHENGINESPRINGERLINK_H
#define SEARCHENGINESPRINGERLINK_H

#include <QObject>

#include "searchengineabstract.h"

class NetworkAccessManager;

/**
 * Search engine to query SpringerLink for files.
 *
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class SearchEngineSpringerLink : public SearchEngineAbstract
{
    Q_OBJECT
public:
    static const QString AllCategories;
    static const int AllYears;
    static const QString AllContentTypes;
    static const QString AllSubjects;

    explicit SearchEngineSpringerLink(NetworkAccessManager *networkAccessManager, const QString &searchTerm, const QString &category = AllCategories, const QString &contentType = AllContentTypes, const QString &subject = AllSubjects, int year = AllYears, QObject *parent = nullptr);

    virtual void startSearch(int numExpectedHits);
    virtual bool isAlive();

private:
    NetworkAccessManager *m_networkAccessManager;
    const QString m_searchTerm;
    const QString m_category;
    const QString m_contentType;
    const QString m_subject;
    const int m_year;
    int m_numExpectedHits, m_numFoundHits;
    int m_searchOffset;
    bool m_isRunning;

    void nextSearchStep();

private slots:
    void finished();
};

#endif // SEARCHENGINESPRINGERLINK_H
