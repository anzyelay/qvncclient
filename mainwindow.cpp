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
    QSplashScreen splash(QPixmap(":/fs.bmp").scaled(this->size()),Qt::WindowStaysOnTopHint);
    splash.show();
    ui->setupUi(this);
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
    int i=1;
    while (!ui->vncView->isConnectedToServer()) {
        QString str(i++,QChar('.'));
        splash.showMessage("启动中，请稍等\n"+str,Qt::AlignBottom|Qt::AlignHCenter);
        usleep(1000);
        qApp->processEvents();
        if(i>30) i=1;
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
