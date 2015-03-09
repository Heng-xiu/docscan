QT += network xml gui
QT -= webkit
SUBDIRS = wv2
WARNINGS += -Wall
TARGET = DocScan
CONFIG += console
CONFIG -= app_bundle
CONFIG += ordered
TEMPLATE = app
DEFINES += HAVE_ICONV_H ICONV_CONST= HAVE_STRING_H HAVE_MATH_H

# for profiling
##QMAKE_CXXFLAGS_DEBUG += -pg
##LIBS += -pg

SOURCES += src/main.cpp \
    src/searchengineabstract.cpp \
    src/searchenginebing.cpp src/downloader.cpp \
    src/fileanalyzerabstract.cpp src/fileanalyzerrtf.cpp  \
    src/fileanalyzerpdf.cpp src/searchenginegoogle.cpp \
    src/fileanalyzerodf.cpp src/watchdog.cpp \
    src/logcollector.cpp src/popplerwrapper.cpp \
    src/general.cpp src/urldownloader.cpp \
    src/fileanalyzeropenxml.cpp \
    src/filefinder.cpp src/fromlogfile.cpp \
    src/filesystemscan.cpp src/filefinderlist.cpp \
    src/webcrawler.cpp src/fakedownloader.cpp \
    src/fileanalyzermultiplexer.cpp \
    src/fileanalyzercompoundbinary.cpp \
    src/geoip.cpp \
    src/searchenginespringerlink.cpp \
    src/networkaccessmanager.cpp \
    src/guessing.cpp
HEADERS += src/searchengineabstract.h \
    src/searchenginebing.h src/downloader.h \
    src/fileanalyzerabstract.h src/searchenginegoogle.h \
    src/fileanalyzerpdf.h src/watchdog.h \
    src/fileanalyzerodf.h src/watchable.h \
    src/logcollector.h src/fromlogfile.h \
    src/general.h src/urldownloader.h \
    src/fileanalyzeropenxml.h src/fileanalyzerrtf.h \
    src/filefinder.h src/popplerwrapper.h \
    src/filesystemscan.h src/filefinderlist.h \
    src/webcrawler.h src/fakedownloader.h \
    src/fileanalyzermultiplexer.h \
    src/fileanalyzercompoundbinary.h \
    src/geoip.h \
    src/searchenginespringerlink.h \
    src/networkaccessmanager.h \
    src/guessing.h

# wv2
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
  src/wv2/headers95.h src/wv2/exceptions.h src/wv2/msdoc.h src/wv2/exceptions.h src/wv2/pole.h

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
  src/wv2/properties97.cpp src/wv2/headers.cpp src/wv2/lists.cpp

# rtf-qt
HEADERS += src/rtf-qt/ManagerPcdataDestination.h src/rtf-qt/DocumentCommentPcdataDestination.h \
  src/rtf-qt/FontTableEntry.h src/rtf-qt/OperatorPcdataDestination.h \
  src/rtf-qt/InfoTimeDestination.h src/rtf-qt/KeywordsPcdataDestination.h \
  src/rtf-qt/InfoRevisedTimeDestination.h src/rtf-qt/AuthorPcdataDestination.h \
  src/rtf-qt/Token.h src/rtf-qt/CommentPcdataDestination.h src/rtf-qt/UserPropsDestination.h \
  src/rtf-qt/GeneratorPcdataDestination.h src/rtf-qt/TextDocumentRtfOutput.h \
  src/rtf-qt/StyleSheetTableEntry.h src/rtf-qt/PictDestination.h src/rtf-qt/rtfreader.h \
  src/rtf-qt/SubjectPcdataDestination.h src/rtf-qt/FontTableDestination.h \
  src/rtf-qt/IgnoredDestination.h src/rtf-qt/CategoryPcdataDestination.h \
  src/rtf-qt/InfoPrintedTimeDestination.h src/rtf-qt/controlword.h \
  src/rtf-qt/Tokenizer.h src/rtf-qt/RtfGroupState.h src/rtf-qt/AbstractRtfOutput.h \
  src/rtf-qt/PcdataDestination.h src/rtf-qt/DocumentDestination.h \
  src/rtf-qt/HLinkBasePcdataDestination.h src/rtf-qt/StyleSheetDestination.h \
  src/rtf-qt/TitlePcdataDestination.h src/rtf-qt/InfoCreatedTimeDestination.h \
  src/rtf-qt/Destination.h src/rtf-qt/CompanyPcdataDestination.h \
  src/rtf-qt/ColorTableDestination.h src/rtf-qt/InfoDestination.h

SOURCES += src/rtf-qt/Tokenizer.cpp src/rtf-qt/IgnoredDestination.cpp \
  src/rtf-qt/InfoTimeDestination.cpp src/rtf-qt/CategoryPcdataDestination.cpp \
  src/rtf-qt/GeneratorPcdataDestination.cpp src/rtf-qt/rtfreader.cpp \
  src/rtf-qt/AuthorPcdataDestination.cpp src/rtf-qt/StyleSheetDestination.cpp \
  src/rtf-qt/KeywordsPcdataDestination.cpp src/rtf-qt/PictDestination.cpp \
  src/rtf-qt/SubjectPcdataDestination.cpp src/rtf-qt/InfoRevisedTimeDestination.cpp \
  src/rtf-qt/DocumentDestination.cpp src/rtf-qt/FontTableDestination.cpp \
  src/rtf-qt/Destination.cpp src/rtf-qt/OperatorPcdataDestination.cpp \
  src/rtf-qt/HLinkBasePcdataDestination.cpp src/rtf-qt/ColorTableDestination.cpp \
  src/rtf-qt/InfoCreatedTimeDestination.cpp src/rtf-qt/Token.cpp \
  src/rtf-qt/InfoDestination.cpp src/rtf-qt/InfoPrintedTimeDestination.cpp \
  src/rtf-qt/UserPropsDestination.cpp src/rtf-qt/controlword.cpp \
  src/rtf-qt/CommentPcdataDestination.cpp src/rtf-qt/TitlePcdataDestination.cpp \
  src/rtf-qt/ManagerPcdataDestination.cpp src/rtf-qt/CompanyPcdataDestination.cpp \
  src/rtf-qt/PcdataDestination.cpp src/rtf-qt/TextDocumentRtfOutput.cpp \
  src/rtf-qt/DocumentCommentPcdataDestination.cpp src/rtf-qt/AbstractRtfOutput.cpp

INCLUDEPATH += src/wv2/generator src/wv2/ src/rtf-qt/


# load and parse zip'ed files (e.g. OpenDocument files)
LIBS += -lquazip

# load compressed files
LIBS += -llzma

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += libgsf-1 glib-2.0 poppler-qt4 poppler-cpp poppler
}
