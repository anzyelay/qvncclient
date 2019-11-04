#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_dialog_setting.h"
#include <QSettings>
#include <QCompleter>
#define LOGIN_SHELL_CMD_STR "killall dt_ui;/tmp/dt_ui --platform vnc&sleep 1;ps | grep dt_ui | grep -v grep && echo connect2vnc"
#define LOGOUT_SHELL_CMD_STR "killall dt_ui;/tmp/dt_ui&sleep 1;ps | grep dt_ui | grep -v grep && echo remote dt_ui is running"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    cfg = new QSettings("./"+qAppName()+".ini",QSettings::IniFormat, this);
    ui->setupUi(this);
    connect(ui->disstyle_btn,&QPushButton::pressed,[=](){
        if(ui->disstyle_btn->text() == "FULL"){
            ui->disstyle_btn->setText("CENTER");
            ui->vncView->setFullScreen(true);
        }
        else {
            ui->disstyle_btn->setText("FULL");
            ui->vncView->setFullScreen(false);
        }
    });
    if(ui->disstyle_btn->text() == "FULL"){
        ui->disstyle_btn->setText("CENTER");
        ui->vncView->setFullScreen(true);
    }
    else {
        ui->disstyle_btn->setText("FULL");
        ui->vncView->setFullScreen(false);
    }
    connect(&ssh,&QSshSocket::error,[&](QSshSocket::SshError se){
        ui->statusBar->showMessage(tr("error :%1").arg(se));
    });
    connect(&ssh,&QSshSocket::commandExecuted,this,[=](QString s1,QString s2){
        if(!s1.isEmpty())
            ui->textEdit->append(ssh.user()+"@"+ssh.host()+">>"+s1);
        if(!s2.isEmpty())
            ui->textEdit->append(s2);
        ui->textEdit->moveCursor(QTextCursor::End);
        if(s2.contains("connect2vnc")){
            ui->vncView->connectToVncServer(cfg->value("hostip").toString(), "");
            ui->vncView->startFrameBufferUpdate();
            ui->restart_btn->setText("断开远程");
            ui->restart_btn->setEnabled(true);
        }
    });
    connect(&ssh,&QSshSocket::loginSuccessful,this,[=](){
        ui->statusBar->showMessage(tr("Connected to %1 , <UserName>: %2").arg(ssh.host()).arg(ssh.user()));
        QString cmdstr = cfg->value("cmd").toString();
        ssh.loginInteractiveShell();
        if(!cmdstr.isEmpty())
            ssh.add2ShellCommand(cfg->value("cmd").toString());
    });
    connect(ui->setting_btn,&QPushButton::clicked,[=](){
        QDialog dialog;
        Ui::Dialog setting_ui;
        setting_ui.setupUi(&dialog);
        setting_ui.hostip->setText(cfg->value("hostip","192.168.200.166").toString());
        setting_ui.port->setText(cfg->value("port","22").toString());
        setting_ui.username->setText(cfg->value("username","root").toString());
        setting_ui.passwd->setText(cfg->value("passwd","").toString());
        setting_ui.cmd->setText(cfg->value("cmd",LOGIN_SHELL_CMD_STR).toString());
        setting_ui.exitCmd->setText(cfg->value("exitCmd",LOGOUT_SHELL_CMD_STR).toString());
        if(dialog.exec()==QDialog::Accepted){
            cfg->setValue("hostip",setting_ui.hostip->text());
            cfg->setValue("port",setting_ui.port->text());
            cfg->setValue("username",setting_ui.username->text());
            cfg->setValue("passwd",setting_ui.passwd->text());
            cfg->setValue("cmd",setting_ui.cmd->text());
            cfg->setValue("exitCmd",setting_ui.exitCmd->text());
        }
    });
    ui->textEdit->hide();
    ui->cmd_edit->hide();
    ui->frame->hide();
    QStringList strs = cfg->value("cmdhistory","ls").toStringList();
    auto cmdcompleter = new QCompleter(strs, ui->cmd_edit);
    cmdcompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->cmd_edit->setCompleter(cmdcompleter);
}

MainWindow::~MainWindow()
{
    cfg->setValue("cmdhistory",dynamic_cast<QStringListModel *>(ui->cmd_edit->completer()->model())->stringList());
    ui->vncView->disconnectFromVncServer();
    ssh.disconnectFromHost();
    delete ui;
}

void MainWindow::on_connect_btn_pressed()
{
    if(!ssh.isLoggedIn())
    {
        QString hostip,username,passwd;
        username = cfg->value("username").toString();
        passwd = cfg->value("passwd").toString();
        hostip = cfg->value("hostip").toString();
        if(hostip.isEmpty()||username.isEmpty()){
            QMessageBox::warning(this,"warn","set arguments first");
            return;
        }
        ssh.setConnectHost(cfg->value("hostip").toString());
        ssh.login(username, passwd);
    }
}


void MainWindow::on_disconnect_btn_pressed()
{
    if(ui->vncView->isConnectedToServer())
        ui->vncView->disconnectFromVncServer();
    ssh.disconnectFromHost();
    ui->statusBar->showMessage(tr("disconnected"));
}

void MainWindow::on_cmd_edit_returnPressed()
{
    if(ssh.isLoggedIn())
        ssh.add2ShellCommand(ui->cmd_edit->text());
    QStringList strs = dynamic_cast<QStringListModel *>(ui->cmd_edit->completer()->model())->stringList();
    if(!strs.contains(ui->cmd_edit->text()))
    {
        if(strs.size()>100)
            strs.pop_front();
        strs << ui->cmd_edit->text();
        dynamic_cast<QStringListModel *>(ui->cmd_edit->completer()->model())->setStringList(strs);
    }
}

void MainWindow::on_toolButton_cmd_pressed()
{
    bool show = ui->cmd_edit->isVisible();
    ui->cmd_edit->setHidden(show);
    ui->textEdit->setHidden(show);
}

void MainWindow::on_toolButton_set_pressed()
{
    ui->frame->setVisible(!ui->frame->isVisible());
}


void MainWindow::on_restart_btn_clicked()
{
    QString str = ui->restart_btn->text();
    if(str == "断开远程"){
        if(ui->vncView->isConnectedToServer())
            ui->vncView->disconnectFromVncServer();
        if(!ssh.isLoggedIn())
            return;
        ssh.add2ShellCommand(cfg->value("exitCmd").toString());
        ui->restart_btn->setText("远程控制");
    }
    else{
        if(!ssh.isLoggedIn())
            return;
        if(!cfg->value("cmd").toString().isEmpty()){
            ssh.add2ShellCommand(cfg->value("cmd").toString());
            ui->restart_btn->setEnabled(false);
        }
    }
}
