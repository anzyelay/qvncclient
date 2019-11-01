
QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QVNCClient
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    qsshsocket.cpp \
    qvncclientwidget.cpp

HEADERS  += mainwindow.h \
    qsshsocket.h \
    qvncclientwidget.h

FORMS    += mainwindow.ui \
    dialog_setting.ui

LIBS += -lssh
win32{
# static lib request this
    DEFINES += LIBSSH_STATIC

    INCLUDEPATH += $$PWD/lib
    DEPENDPATH += $$PWD/lib
    LIBS += -lgcrypt -lgpg-error
    LIBS += -lws2_32 -lz
}



