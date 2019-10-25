#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->disstyle_cbx,&QCheckBox::stateChanged,ui->vncView,&QVNCClientWidget::setFullScreen);
    ui->vncView->setFullScreen(ui->disstyle_cbx->isChecked());
    connect(&ssh,&QSshSocket::error,[&](QSshSocket::SshError se){
        qDebug() << se;
    });
    connect(&ssh,&QSshSocket::commandExecuted, [=](QString s1,QString s2){
        qDebug() << s1;
        ui->label_2->setText(s2);
    });
}

MainWindow::~MainWindow()
{
    ssh.disconnectFromHost();
    delete ui;
}

void MainWindow::on_connect_btn_pressed()
{
    ui->vncView->connectToVncServer(ui->host_edit->text(), "");
    ui->vncView->startFrameBufferUpdate();
}


void MainWindow::on_disconnect_btn_pressed()
{
    ui->vncView->disconnectFromVncServer();
}

void MainWindow::on_cmd_edit_returnPressed()
{
    if(!ssh.isConnected())
        ssh.connectToHost(ui->host_edit->text());
    if(!ssh.isLoggedIn())
    {
        QString usrname = QInputDialog::getText(this, "Username", "Enter login name:", QLineEdit::Normal);
        QString passwd = QInputDialog::getText(this, "Password", "Enter password:", QLineEdit::Password);
        ssh.login(usrname, passwd);
    }
    ssh.executeCommand(ui->cmd_edit->text());
    ui->label_2->clear();
}
