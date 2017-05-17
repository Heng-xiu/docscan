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

#ifndef SEARCHENGINEABSTRACT_H
#define SEARCHENGINEABSTRACT_H

#include <QObject>

#include "filefinder.h"

/**
 * @author Thomas Fischer <thomas.fischer@his.se>
 */
class SearchEngineAbstract : public FileFinder
{
    Q_OBJECT

public:
    explicit SearchEngineAbstract(QObject *parent = nullptr);
};

#endif // SEARCHENGINEABSTRACT_H
