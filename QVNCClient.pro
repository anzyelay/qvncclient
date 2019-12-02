
QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += C++11

TARGET = dt_vnc
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
#    qanntextedit.cpp \
#    qsshsocket.cpp \
    qvncclientwidget.cpp

HEADERS  += mainwindow.h \
#    qanntextedit.h \
#    qsshsocket.h \
    qvncclientwidget.h

FORMS    += mainwindow.ui \
#    dialog_setting.ui

#win32{
#        # static lib request this
#        DEFINES += LIBSSH_STATIC
#        LIBS += -L$$PWD/lib -lssh -lgcrypt -lgpg-error
#        LIBS += -lws2_32 -lz
#}
#else:LIBS += -lssh
target.path = /tmp
INSTALLS += target


