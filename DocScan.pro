# -------------------------------------------------
# Project created by QtCreator 2011-05-09T17:37:17
# -------------------------------------------------
QT += network \
    webkit
QT -= gui
TARGET = DocScan
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += src/main.cpp \
    src/searchengineabstract.cpp \
    src/searchenginebing.cpp src/downloader.cpp
HEADERS += src/searchengineabstract.h \
    src/searchenginebing.h src/downloader.h
