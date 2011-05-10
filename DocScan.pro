# -------------------------------------------------
# Project created by QtCreator 2011-05-09T17:37:17
# -------------------------------------------------
QT += network webkit xml
QT -= gui
TARGET = DocScan
CONFIG += console
CONFIG -= app_bundle thread
TEMPLATE = app
SOURCES += src/main.cpp \
    src/searchengineabstract.cpp \
    src/searchenginebing.cpp src/downloader.cpp \
    src/fileanalyzerabstract.cpp \
    src/fileanalyzerpdf.cpp src/searchenginegoogle.cpp \
    src/fileanalyzerodf.cpp
HEADERS += src/searchengineabstract.h \
    src/searchenginebing.h src/downloader.h \
    src/fileanalyzerabstract.h src/searchenginegoogle.h \
    src/fileanalyzerpdf.h \
    src/fileanalyzerodf.h

# load and parse PDF files
unix:!macx:!symbian: LIBS += -lpoppler-qt4

# load and parse zip'ed files (e.g. OpenDocument files)
unix:!macx:!symbian: LIBS += -lquazip
