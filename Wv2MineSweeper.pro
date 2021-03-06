QT -= gui webkit network xml
SUBDIRS = wv2
WARNINGS += -Wall
TARGET = Wv2MineSweeper
CONFIG += console
CONFIG -= app_bundle thread
CONFIG += ordered
TEMPLATE = app
DEFINES += HAVE_WV2 HAVE_ICONV_H ICONV_CONST= HAVE_STRING_H HAVE_MATH_H

SOURCES += src/wv2minesweeper.cpp src/fileanalyzercompoundbinary.cpp \
  src/fileanalyzerabstract.cpp src/general.cpp src/poorlogger.cpp
HEADERS += src/fileanalyzercompoundbinary.h src/fileanalyzerabstract.h \
  src/general.h src/poorlogger.h

# wv2
HEADERS += src/wv2/word95_helper.h src/wv2/global.h src/wv2/word_helper.h src/wv2/styles.h \
  src/wv2/associatedstrings.h src/wv2/word97_helper.h src/wv2/bookmark.h src/wv2/utilities.h \
  src/wv2/handlers.h src/wv2/convert.h src/wv2/annotations.h  src/wv2/parser9x.h \
  src/wv2/headers97.h src/wv2/wv2version.h src/wv2/olestream.h src/wv2/properties97.h \
  src/wv2/parserfactory.h src/wv2/parser.h src/wv2/parser95.h src/wv2/textconverter.h \
  src/wv2/graphics.h src/wv2/fonts.h src/wv2/ms_odraw.h src/wv2/headers.h \
  src/wv2/generator/template-Word95.h src/wv2/generator/template-Word97.h \
  src/wv2/generator/template-conv.h src/wv2/lists.h src/wv2/wv2_export.h src/wv2/parser97.h \
  src/wv2/crc32.h src/wv2/paragraphproperties.h src/wv2/fields.h src/wv2/functor.h \
  src/wv2/footnotes97.h src/wv2/word95_generated.h src/wv2/ustring.h src/wv2/olestorage.h \
  src/wv2/word97_generated.h src/wv2/wvlog.h src/wv2/functordata.h src/wv2/sharedptr.h \
  src/wv2/headers95.h src/wv2/pole.h

SOURCES += src/wv2/crc32.c src/wv2/handlers.cpp src/wv2/word_helper.cpp \
  src/wv2/parserfactory.cpp src/wv2/parser95.cpp src/wv2/word95_generated.cpp \
  src/wv2/functor.cpp src/wv2/parser9x.cpp src/wv2/graphics.cpp src/wv2/word97_generated.cpp \
  src/wv2/paragraphproperties.cpp src/wv2/convert.cpp src/wv2/headers95.cpp \
  src/wv2/annotations.cpp src/wv2/headers97.cpp src/wv2/textconverter.cpp \
  src/wv2/olestream.cpp src/wv2/parser.cpp src/wv2/functordata.cpp src/wv2/olestorage.cpp \
  src/wv2/utilities.cpp src/wv2/wv2version.cpp \
  src/wv2/fields.cpp src/wv2/associatedstrings.cpp src/wv2/word95_helper.cpp \
  src/wv2/word97_helper.cpp src/wv2/parser97.cpp src/wv2/styles.cpp src/wv2/bookmark.cpp \
  src/wv2/global.cpp src/wv2/ustring.cpp src/wv2/fonts.cpp src/wv2/footnotes97.cpp \
  src/wv2/properties97.cpp src/wv2/headers.cpp src/wv2/lists.cpp src/wv2/pole.cpp

INCLUDEPATH += src/wv2/generator src/wv2/


unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += libgsf-1 glib-2.0
}

