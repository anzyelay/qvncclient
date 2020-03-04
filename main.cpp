#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qApp->setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication a(argc, argv);
    MainWindow w;
    w.showMaximized();
//    w.show();

    return a.exec();
}
