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

#include <QCoreApplication>
#include <QDebug>

#include "fileanalyzercompoundbinary.h"

std::ofstream debugFile("/tmp/wvlog-minesweeper.txt");

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (argc == 2) {
        const QString filename = QString::fromAscii(argv[argc - 1]);
        FileAnalyzerCompoundBinary facb(&a);
        qDebug() << "Minesweeping file" << filename;
        facb.analyzeFile(filename);
        while (facb.isAlive()) {
            qApp->processEvents();
        }
        return 0;
    } else {
        qWarning() << "Needing exactly one filename as parameter";
        return 1;
    }
}
