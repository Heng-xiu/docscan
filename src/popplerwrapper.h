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

#ifndef POPPLERWRAPPER_H
#define POPPLERWRAPPER_H

#include <QString>
#include <QDateTime>
#include <QSize>

namespace poppler
{
class document;
}

/**
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class PopplerWrapper
{
public:
    static PopplerWrapper *createPopplerWrapper(const QString &filename);
    ~PopplerWrapper();

    void getPdfVersion(int &majorVersion, int &minorVersion) const;

    QStringList fontNames() const;
    QString info(const QString &field) const;
    QDateTime date(const QString &field) const;

    int numPages() const;
    QString plainText() const;
    QSizeF pageSize() const;

    QString imagesLog();

    bool isLocked() const;
    bool isEncrypted() const;

protected:
    PopplerWrapper(poppler::document *document, const QString &filename);

private:
    poppler::document *m_document;
    QString m_filename;
};

#endif // POPPLERWRAPPER_H
