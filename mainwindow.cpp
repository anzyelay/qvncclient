#include "qanntextedit.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCompleter>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    auto t = new QTimer(this);
    connect(t, &QTimer::timeout, [=](){
        if(ui->vncView->isConnectedToServer()) return;
        ui->vncView->disconnectFromVncServer();
//        ui->vncView->connectToVncServer("192.168.200.166", "");
        bool isconnect = ui->vncView->connectToVncServer("127.0.0.1", "");
        if(isconnect){
            ui->vncView->startFrameBufferUpdate();
            ui->vncView->setFocus();
//            ui->vncView->grabKeyboard();
            t->stop();
            t->start(5000);
        }
    });
    t->start(200);
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
