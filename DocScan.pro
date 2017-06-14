# Run  qmake CONFIG+=quazip5  to enable support for both ODF and OpenXML formats
# Run  qmake CONFIG+=wv2  to enable support for historic Word file formats
# Run  qmake "CONFIG+=wv2 quazip5"  to enable all features

QT += network xml gui
QT -= webkit

wv2 {
    SUBDIRS = wv2
    DEFINES += HAVE_WV2
}

WARNINGS += -Wall
TARGET = DocScan
CONFIG += console
CONFIG -= app_bundle
CONFIG += ordered
CONFIG += c++11
TEMPLATE = app
DEFINES += HAVE_ICONV_H ICONV_CONST= HAVE_STRING_H HAVE_MATH_H

# for profiling
##QMAKE_CXXFLAGS_DEBUG += -pg
##LIBS += -pg

SOURCES += src/main.cpp src/watchdog.cpp \
    src/searchengineabstract.cpp \
    src/searchenginebing.cpp src/downloader.cpp \
    src/fileanalyzerabstract.cpp src/fileanalyzerjpeg.cpp \
    src/fileanalyzerpdf.cpp src/searchenginegoogle.cpp \
    src/logcollector.cpp src/jhovewrapper.cpp \
    src/general.cpp src/urldownloader.cpp \
    src/filefinder.cpp src/fromlogfile.cpp \
    src/filesystemscan.cpp src/filefinderlist.cpp \
    src/webcrawler.cpp src/fakedownloader.cpp \
    src/fileanalyzermultiplexer.cpp \
    src/geoip.cpp \
    src/searchenginespringerlink.cpp \
    src/networkaccessmanager.cpp \
    src/guessing.cpp
HEADERS += src/searchengineabstract.h \
    src/searchenginebing.h src/downloader.h \
    src/fileanalyzerabstract.h src/searchenginegoogle.h \
    src/fileanalyzerpdf.h src/fileanalyzerjpeg.h \
    src/watchdog.h src/watchable.h \
    src/logcollector.h src/fromlogfile.h \
    src/general.h src/urldownloader.h \
    src/filefinder.h src/jhovewrapper.h \
    src/filesystemscan.h src/filefinderlist.h \
    src/webcrawler.h src/fakedownloader.h \
    src/fileanalyzermultiplexer.h \
    src/geoip.h \
    src/searchenginespringerlink.h \
    src/networkaccessmanager.h \
    src/guessing.h

wv2 {
    SOURCES += src/wv2/crc32.c src/wv2/handlers.cpp src/wv2/word_helper.cpp \
      src/wv2/parserfactory.cpp src/wv2/parser95.cpp src/wv2/word95_generated.cpp \
      src/wv2/functor.cpp src/wv2/parser9x.cpp src/wv2/graphics.cpp src/wv2/word97_generated.cpp \
      src/wv2/paragraphproperties.cpp src/wv2/convert.cpp src/wv2/headers95.cpp \
      src/wv2/annotations.cpp src/wv2/headers97.cpp src/wv2/textconverter.cpp \
      src/wv2/olestream.cpp src/wv2/parser.cpp src/wv2/functordata.cpp src/wv2/olestorage.cpp \
      src/wv2/utilities.cpp src/wv2/wv2version.cpp src/wv2/pole.cpp src/wv2/wvlog.cpp \
      src/wv2/fields.cpp src/wv2/associatedstrings.cpp src/wv2/word95_helper.cpp \
      src/wv2/word97_helper.cpp src/wv2/parser97.cpp src/wv2/styles.cpp src/wv2/bookmark.cpp \
      src/wv2/global.cpp src/wv2/ustring.cpp src/wv2/fonts.cpp src/wv2/footnotes97.cpp \
      src/wv2/properties97.cpp src/wv2/headers.cpp src/wv2/lists.cpp \
      src/fileanalyzercompoundbinary.cpp

    HEADERS += src/wv2/word95_helper.h src/wv2/global.h src/wv2/word_helper.h src/wv2/styles.h \
      src/wv2/associatedstrings.h src/wv2/word97_helper.h src/wv2/bookmark.h src/wv2/utilities.h \
      src/wv2/handlers.h src/wv2/convert.h src/wv2/annotations.h  src/wv2/parser9x.h \
      src/wv2/headers97.h src/wv2/wv2version.h src/wv2/olestream.h src/wv2/properties97.h \
      src/wv2/parserfactory.h src/wv2/parser.h src/wv2/parser95.h src/wv2/textconverter.h \
      src/wv2/graphics.h src/wv2/fonts.h src/wv2/headers.h \
      src/wv2/lists.h src/wv2/wv2_export.h src/wv2/parser97.h \
      src/wv2/crc32.h src/wv2/paragraphproperties.h src/wv2/fields.h src/wv2/functor.h \
      src/wv2/footnotes97.h src/wv2/word95_generated.h src/wv2/ustring.h src/wv2/olestorage.h \
      src/wv2/word97_generated.h src/wv2/wvlog.h src/wv2/functordata.h src/wv2/sharedptr.h \
      src/wv2/headers95.h src/wv2/exceptions.h src/wv2/msdoc.h src/wv2/exceptions.h src/wv2/pole.h \
      src/fileanalyzercompoundbinary.h

    INCLUDEPATH += src/wv2/generator src/wv2/
}


quazip5 {
    # load and parse zip'ed files (e.g. OpenDocument files)
    exists( /usr/lib/libquazip5.so ) {
        QUAZIP5LIBPATH = /usr/lib/
    }
    exists( /usr/lib/x86_64-linux-gnu/libquazip5.so ) {
        QUAZIP5LIBPATH = /usr/lib/x86_64-linux-gnu
    }
    isEmpty( QUAZIP5LIBPATH ) {
        error( "Could not find Quazip5's library" )
    }
    message( "Found Quazip5's library in '"$$QUAZIP5LIBPATH"'" )
    LIBS += -lquazip5 -L$$QUAZIP5LIBPATH

    exists( /usr/include/quazip/quazip.h ) {
        message( "Found Quazip5's headers in '/usr/include/quazip'" )
        INCLUDEPATH += /usr/include/quazip
    }
    exists( /usr/include/quazip5/quazip.h ) {
        message( "Found Quazip5's headers in '/usr/include/quazip5'" )
        INCLUDEPATH += /usr/include/quazip5
    }

    DEFINES += HAVE_QUAZIP5

    SOURCES += src/fileanalyzeropenxml.cpp src/fileanalyzerodf.cpp
    HEADERS += src/fileanalyzeropenxml.h src/fileanalyzerodf.h
}

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += poppler-qt5
}
