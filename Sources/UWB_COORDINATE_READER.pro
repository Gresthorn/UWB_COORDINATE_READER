#-------------------------------------------------
#
# Project created by QtCreator 2014-12-25T23:30:04
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UWB_COORDINATE_READER
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    server_pipe.cpp \
    uwbpacketclass.cpp \
    rs232.c

HEADERS  += mainwindow.h \
    server_pipe.h \
    rs232.h \
    uwbpacketclass.h

FORMS    += mainwindow.ui

RESOURCES += \
    icons.qrc

RC_FILE = iconrc.rc
