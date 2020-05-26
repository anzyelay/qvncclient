#include "qanntextedit.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCompleter>
#include <QTimer>
#include <QSplashScreen>

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QPixmap pixmap("/tmp/logo.bmp");
    if(pixmap.isNull()){
        pixmap = QPixmap(this->size());
        pixmap.fill(Qt::black);
    }
    else {
        pixmap = pixmap.scaled(this->size());
    }
    QSplashScreen splash(pixmap,Qt::WindowStaysOnTopHint);
    splash.show();
    auto t = new QTimer(this);
    connect(t, &QTimer::timeout, [=](){
        if(ui->vncView->isConnectedToServer()) return;
        ui->vncView->disconnectFromVncServer();
        bool isconnect = ui->vncView->connectToVncServer("127.0.0.1");
        if(isconnect){
            ui->vncView->startFrameBufferUpdate();
            ui->vncView->setFocus();
            t->stop();
            t->start(5000);
        }
    });
    t->start(200);
    int i=70;
    splash.setFont(QFont("msyh", 14));
    while (!ui->vncView->isConnectedToServer()) {
        QString str(i++,QChar('='));
        splash.showMessage("正在启动，请稍等\n"+str,Qt::AlignBottom|Qt::AlignHCenter, Qt::green);
        usleep(100000);
        qApp->processEvents();
        if(i>71) i=70;
    }
    splash.finish(parent);
}

MainWindow::~MainWindow()
{
    ui->vncView->disconnectFromVncServer();
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
//    if(e->type() == QKeyEvent::KeyPress ){
//        QKeyEvent *keyevent = static_cast<QKeyEvent *>(e);
//        if(keyevent->modifiers() == Qt::ControlModifier){
//            if( keyevent->key() == 	Qt::Key_C ){
//                return true;
//            }
//        }
//    }
    return QMainWindow::eventFilter(obj, e);
}
