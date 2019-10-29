
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

FORMS    += mainwindow.ui


win32{
# static lib request this
#    DEFINES += LIBSSH_STATIC

    INCLUDEPATH += $$PWD/lib
    DEPENDPATH += $$PWD/lib
    LIBS += -L$$PWD/lib/
}

LIBS += -lssh


