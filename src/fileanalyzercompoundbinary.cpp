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

#include <QFile>
#include <QDebug>
#include <QtEndian>

#include "fileanalyzercompoundbinary.h"

enum DirectoryEntryType {
    Empty = 0, UserStorage = 1, UserStream = 2, LockBytes = 3, Property = 4, RootStorage = 5
};
enum NodeColor {
    Red = 0, Black = 1
};

struct cfdirentry {
    QString name;
    DirectoryEntryType entryType;
    NodeColor nodeColor;
    qint32 leftChildDirID, rightChildDirID, rootNodeDirID;
    quint64 timeStampCreation, timeStampModification;
    qint32 firstSecID;
    quint32 size;
};

struct cfheader {
    quint8 uid[16];
    quint16 revisionNumber;
    quint16 versionNumber;
    int sectorSize;
    int shortSectorSize;
    quint32 numSectorsUsedInSAT;
    qint32 secIDofFirstSectorDirectoryStream;
    quint32 minSizeStandardStream;
    qint32 secIDofFirstSSAT;
    quint32 numSectorsUsedInSSAT;
    qint32 secIDofFirstMSAT;
    quint32 numSectorsUsedInMSAT;
};

struct compoundfile {
    struct cfheader *header;
};

struct cfheader *readHeader(QIODevice *device) {
    device->seek(0);

    {
        /// read and check compound document file identifier
        /// D0 CF 11 E0 A1 B1 1A E1
        quint32 a32 = 0, b32 = 0;
        device->read((char *)(&a32), 4);
        device->read((char *)(&b32), 4);

        if (a32 != 0xe011cfd0 || b32 != 0xe11ab1a1)
            return NULL;
    }

    struct cfheader *result = new struct cfheader;

    /// read UID
    device->read((char *)result->uid, 16);
    /// read revision number
    device->read((char *) & (result->revisionNumber), 2);
    /// read version number
    device->read((char *) & (result->versionNumber), 2);

    {
        /// read endianess
        quint8 buffer[2];
        device->read((char *)buffer, 2);
        // TODO check for correct endianess
    }

    {
        /// read sector sizes
        quint16 buffer;

        device->read((char *)&buffer, 2);
        result->sectorSize = 1 << buffer;

        device->read((char *)&buffer, 2);
        result->shortSectorSize = 1 << buffer;
    }

    {
        /// skip 10 bytes
        quint16 buffer[8];
        device->read((char *)&buffer, 10);
    }

    {
        /// read number of sectors, first sectors, and minimum size
        quint32 unused;
        device->read((char *) & (result->numSectorsUsedInSAT), 4);
        device->read((char *) & (result->secIDofFirstSectorDirectoryStream), 4);
        device->read((char *)&unused, 4);
        device->read((char *) & (result->minSizeStandardStream), 4);
        device->read((char *) & (result->secIDofFirstSSAT), 4);
        device->read((char *) & (result->numSectorsUsedInSSAT), 4);
        device->read((char *) & (result->secIDofFirstMSAT), 4);
        device->read((char *) & (result->numSectorsUsedInMSAT), 4);
    }

    return result;
}

int sectorFileOffset(int secID, struct cfheader *header)
{
    Q_ASSERT(secID >= 0);
    return 512 + secID * header->sectorSize;
}

qint32 *readMSAT(QIODevice *device, struct cfheader *header)
{
    const quint32 sectorIDperSector = (header->sectorSize - 4) / 4;
    qint32 *result = new qint32[header->numSectorsUsedInMSAT * sectorIDperSector + 110];

    {
        /// read first 109 secIDs
        device->seek(76);
        device->read((char *)result, 109);
    }

    int pos = 109;
    qint32 secID = header->secIDofFirstMSAT;
    for (quint32 i = 0; secID >= 0 && i < header->numSectorsUsedInMSAT; ++i) {
        device->seek(sectorFileOffset(secID, header));
        device->read((char *)(result + pos), header->sectorSize - 4);
        pos += sectorIDperSector;
        device->read((char *)&secID, 4);
    }

    return result;
}

qint32 *readSAT(QIODevice *device, struct cfheader *header, qint32 *msat)
{
    const quint32 sectorIDperSector = header->sectorSize / 4;
    int countSectors = 0;
    while (msat[countSectors] >= 0) ++countSectors;
    qint32 *sat = new qint32[countSectors * sectorIDperSector];

    for (int i = 0; i < countSectors; ++i) {
        device->seek(sectorFileOffset(msat[i], header));
        device->read((char *)(sat + i * sectorIDperSector), header->sectorSize);
    }

    return sat;
}

qint32 *readSSAT(QIODevice *device, struct cfheader *header, qint32 *sat)
{
    const quint32 sectorIDperSector = header->sectorSize / 4;
    qint32 curPos = header->secIDofFirstSSAT;
    int countSectors = 0;
    while (curPos >= 0) {
        curPos = sat[curPos];
        ++countSectors;
    }
    qint32 *ssat = new qint32[countSectors * sectorIDperSector];

    curPos = header->secIDofFirstSSAT;
    for (int i = 0; i < countSectors; ++i) {
        device->seek(sectorFileOffset(curPos, header));
        curPos = sat[curPos];
        device->read((char *)(ssat + i * sectorIDperSector), header->sectorSize);
    }

    /*for (int i=0; i<20&& ssat[i]>=0; i+=4)
        qDebug()<<QString::number(ssat[i],16)<<QString::number(ssat[i+1],16)<<QString::number(ssat[i+2],16)<<QString::number(ssat[i+3],16);
    */
    return ssat;
}

cfdirentry *readDirEntry(QIODevice *device, qint32 startPos)
{
    device->seek(startPos);
    cfdirentry *result = new struct cfdirentry;

    {
        /// read name
        quint16 nameBuffer[32];
        quint16 nameLen;
        device->read((char *)nameBuffer, 64);
        device->read((char *)&nameLen, 2);
        result->name = QString::fromUtf16(nameBuffer, nameLen / 2 - 1);
    }

    {
        /// read type of entry and color
        quint8 byte;
        device->read((char *)&byte, 1);
        result->entryType = (DirectoryEntryType)byte;
        device->read((char *)&byte, 1);
        result->nodeColor = (NodeColor)byte;
    }

    {
        /// read right and left dir node ide and root node dir id
        device->read((char *) & (result->leftChildDirID), 4);
        device->read((char *) & (result->rightChildDirID), 4);
        device->read((char *) & (result->rootNodeDirID), 4);
    }

    {
        /// skipping 20 bytes
        quint32 buffer[5];
        device->read((char *)buffer, 20);
    }

    {
        /// read creation and modification time stamp
        device->read((char *) & (result->timeStampCreation), 8);
        device->read((char *) & (result->timeStampModification), 8);
    }

    {
        /// read secID of first sector and total size
        device->read((char *) & (result->firstSecID), 4);
        device->read((char *) & (result->size), 4);
    }

    {
        /// skipping 4 bytes
        quint32 buffer;
        device->read((char *)&buffer, 4);
    }

    return result;
}

FileAnalyzerCompoundBinary::FileAnalyzerCompoundBinary(QObject *parent)
    : FileAnalyzerAbstract(parent)
{
}

bool FileAnalyzerCompoundBinary::isAlive()
{
    return false;
}

void FileAnalyzerCompoundBinary::analyzeFile(const QString &filename)
{
    QFile compoundFileHandle(filename);
    if (compoundFileHandle.open(QFile::ReadOnly)) {
        struct cfheader *header = readHeader(&compoundFileHandle);

        if (header != NULL) {
            qint32 *msat = readMSAT(&compoundFileHandle, header);
            if (msat != NULL) {
                qint32 *sat = readSAT(&compoundFileHandle, header, msat);
                if (sat != NULL) {
                    qint32 *ssat = readSSAT(&compoundFileHandle, header, sat);
                    if (ssat != NULL) {
                        struct cfdirentry *direntry = readDirEntry(&compoundFileHandle, sectorFileOffset(header->secIDofFirstSectorDirectoryStream, header));
                        if (direntry != NULL) {

                            delete direntry;
                        }

                        delete[] ssat;
                    }
                    delete[] sat;
                }
                delete[] msat;
            }
            delete header;
        }
        compoundFileHandle.close();
    }
    // TODO
}
